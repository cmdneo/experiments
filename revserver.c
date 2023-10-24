/**
 * @file revserver.c
 * @author Meeeeeeeeeeeeeee
 * @brief An asynchronous ipv4-TCP-socket server
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>

#define LOG_ERROR(msg) \
	fprintf(stderr, "%s:%d ERROR %s\n", __func__, __LINE__, (msg))

#define INFO(...) printf(__VA_ARGS__)

#define TO_SADDRP(addrp) ((struct sockaddr *)(addrp))

#define IS_ASYNC_ERR(e) (e == EAGAIN || e == EWOULDBLOCK)

/* Define a very limited version of coroutines using the switch statement.
 * Following should be noted:
 * 1: All CORO_AWAIT's should be placed between CORO_BEGIN and CORO_END.
 * 2: All declarations between CORO_BEGIN and CORO_END should be placed inside a block.
 * 3: io_call argument of CORO_AWAIT should evaluate to a negative value and
 *    set errno to EAGAIN or EWOULDBLOCK to indicate that no data is available,
 *    Linux IO system-calls follow this convention.
 * 4: Local variables inside the function using CORO_AWAIT are not preserved once
 *    after the function is suspend by CORO_AWAIT.
 * 5: The step argument of CORO_AWAIT's should be unique and in ascending order
 *    and should not be equal to 0 or UINT_MAX inside a coroutine. just start from 1 /-(^ ^)-/.
 * 6: The step state tracker should be declared using the CORO_STEP macro as the
 *    first argument of the async function, like: some_async_handler(CORO_STEP, int fd) { ... }.
 */
#define CORO_STEP unsigned *cstep____

#define CORO_BEGIN()      \
	switch (*cstep____) { \
	case 0:;

#define CORO_AWAIT(step, ret_var, io_call)            \
	case step:                                        \
		do {                                          \
			*cstep____ = step;                        \
			ret_var = io_call;                        \
			if (ret_var == -1 && IS_ASYNC_ERR(errno)) \
				return -1;                            \
		} while (0)

#define CORO_END()        \
	default:              \
		*cstep____ = ~0U; \
		}

enum {
	BACKLOG = 1024,
	MAX_ACTIVE = 1024,
	MAX_EVENTS = 64,
	BUFFER_SIZE = 4096,
};

enum coro_status {
	CORO_PENDING = -1,
	CORO_DONE = 0,
	CORO_FAIL = 1,
};

typedef struct buffered_reader {
	char buf[BUFFER_SIZE];
	int at;
	int fd;
} buffered_reader;

int buf_read(buffered_reader *r) {

}

typedef struct coroutine {
	int fd; /* FD returned by accept and associated with fin field */
	buffered_reader buffer;
	unsigned step;
	FILE *fin;
	FILE *fout;
	int (*fn)(unsigned *, FILE *, FILE *);
} coroutine;

static bool interrupted = false;

void exit_cleanup(void)
{
	if (interrupted)
		INFO("Interrupt recieved\n");

	// Close everything except STDIN(0), STDOUT(1) and STDERR(2)
	INFO("Closing all file-descriptors\n");
	syscall(__NR_close_range, 3U, ~0U, 0U);
	INFO("Exit!\n");
}

static void sigint_handler(int sig)
{
	if (sig == SIGINT) {
		interrupted = true;
		exit(1);
	}

	assert(!"Invalid signal handler called, sigint only");
}

uint32_t make_ipv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
	return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | (uint32_t)d;
}

void die(const char *msg)
{
	perror(msg);
	exit(1);
}

int setnonblocking(int fd) { return fcntl(fd, F_SETFL, O_NONBLOCK); }

void debug_ipv4_addr(struct sockaddr_in *addrp)
{
	struct sockaddr_in addr = *addrp;
	uint8_t *ipv4 = (uint8_t *)&addr.sin_addr.s_addr;

	INFO("%" PRIu8, ipv4[0]);
	for (int i = 1; i < 4; ++i)
		INFO(".%" PRIu8, ipv4[i]);
	INFO(":%" PRIu16, ntohs(addr.sin_port));
}

static int ipv4_server(struct sockaddr_in *saddr)
{
	// ipv4 TCP socket
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
		die("socket");
	int value = 1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

	if (bind(sfd, TO_SADDRP(saddr), sizeof *saddr) < 0)
		die("bind");
	if (listen(sfd, BACKLOG) < 0)
		die("listen");

	INFO("[LISTEN] ");
	debug_ipv4_addr(saddr);
	INFO("\n");

	return sfd;
}

/* Stop accepting data, also triggers EPOLLRDHUP if registered with epoll */
static void client_shutdown(FILE *fin) { shutdown(fileno(fin), SHUT_RD); }

void close_connection(int rfd, int wfd)
{
	struct sockaddr_in addr;
	socklen_t ssz = sizeof(addr);
	getpeername(rfd, TO_SADDRP(&addr), &ssz);

	INFO("[CLOSED] ");
	debug_ipv4_addr(&addr);
	INFO("\n");
	close(rfd);
	close(wfd);
}

int handle_async_conn(CORO_STEP, FILE *fin, FILE *fout)
{
	int c = ' ';

	CORO_BEGIN();

	while (c != EOF) {
		CORO_AWAIT(1, c, fgetc(fin));
		c = toupper(c);
		fputc(c, fout);

		if (c == '\n') {
			time_t t = time(NULL);
			char *time_str = ctime(&t);
			*strchr(time_str, '\n') = '\0';
			fprintf(fout, "<<<  Time is: %s >>>\n", time_str);
			break;
		}
	}

	CORO_END();
	client_shutdown(fin);

	return 0;
}

static inline int call_coro(coroutine c) { return (c.fn)(&c.step, c.fin, c.fout); }

static coroutine clients[MAX_ACTIVE * 2 + 3]; // 2FD per client and other ones!?

void handle_epoll_event(int efd, int sfd, struct epoll_event ev)
{
	// If event on an already established connection
	if (ev.data.fd != sfd) {
		// Connection dropped
		if (ev.events & EPOLLRDHUP) {
			coroutine c = clients[ev.data.fd];
			close_connection(fileno(c.fin), fileno(c.fout));
			/* Closed FDs are auto-removed from epoll interest list */
		} 
		// New data recieved
		else if (ev.events & EPOLLIN) {
			call_coro(clients[ev.data.fd]);
		} else {
			die("epoll_event: conn_sock");
		}
		return;
	}

	// If server fd then try to accept connection
	struct sockaddr_in saddr;
	socklen_t ssz = sizeof saddr;
	int cfd = accept(sfd, TO_SADDRP(&saddr), &ssz);

	setnonblocking(cfd);
	ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
	ev.data.fd = cfd;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev) < 0)
		die("epoll_ctl: conn_sock");

	clients[cfd] = (coroutine){
		.fd = cfd,
		.fin = fdopen(cfd, "rb"),
		.fout = fdopen(dup(cfd), "wb"),
		.fn = handle_async_conn,
	};

#ifndef NDEBUG
	setlinebuf(clients[cfd].fin);
	setlinebuf(clients[cfd].fout);
#endif

	INFO("[RECIEV] ");
	debug_ipv4_addr(&saddr);
	INFO("\n");
}

int main(void)
{
	signal(SIGINT, sigint_handler);
	atexit(exit_cleanup);

	struct epoll_event ep_events[MAX_EVENTS];
	struct sockaddr_in saddr = {
		.sin_family = AF_INET,
		.sin_port = htons(4000),
		.sin_addr.s_addr = htonl(INADDR_LOOPBACK),
	};

	int efd = epoll_create1(0);
	if (efd < 0)
		die("epoll_create1");

	struct epoll_event ev;
	int sfd = ipv4_server(&saddr);
	// Setup server socket to be nonblocking and register into epoll
	setnonblocking(sfd);
	ev.events = EPOLLIN;
	ev.data.fd = sfd;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &ev) < 0)
		die("epoll_ctl: server_sock");

	while (1) {
		int nfds = epoll_wait(efd, ep_events, MAX_EVENTS, -1);
		if (nfds < 0)
			die("epoll_wait");

		for (int i = 0; i < nfds; ++i)
			handle_epoll_event(efd, sfd, ep_events[i]);
	}

	// Not really needed for now, but anyways
	close(sfd);
	close(efd);

	return 0;
}

// vim: ts=4 sw=4
