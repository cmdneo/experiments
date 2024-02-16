#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define CMD_NOT_FOUND_ERR 127
#define PATH_MAX 4096
#define ARGS_MAX 255

#define PRINTE(...) fprintf(stderr, __VA_ARGS__)

static jmp_buf interrupt_jmp;

static void ctrlc_handler(int sig)
{
	assert(sig == SIGINT);
	siglongjmp(interrupt_jmp, 1);
}

static double timespec_diff(struct timespec s, struct timespec e)
{
	long nano_diff = e.tv_nsec - s.tv_nsec;
	return e.tv_sec - s.tv_sec + (nano_diff / 1000000000.0);
}

static bool check_if_executable(const char *path)
{
	if (faccessat(AT_FDCWD, path, F_OK, 0) == -1) {
		PRINTE("%s: %s\n", path, strerror(errno));
		return false;
	}

	struct stat path_stat;
	fstatat(AT_FDCWD, path, &path_stat, 0);
	if (S_ISDIR(path_stat.st_mode)) {
		PRINTE("%s: Is a directory\n", path);
		return false;
	}

	if (faccessat(AT_FDCWD, path, X_OK, 0) == -1) {
		PRINTE("%s: Is not executable\n", path);
		return false;
	}

	return true;
}

enum State {
	OUTSIDE,
	DQUOTE,
	SQUOTE,
	NOQUOTE,
};

static int
split_into_args(char *command, int args_max, char *args[static args_max])
{
	int state = OUTSIDE;
	int argc = 0;

	for (char *s = command; s[0] != '\0'; ++s) {
		if (argc == args_max)
			break;

		switch (state) {
		case DQUOTE:
			if (*s == '"') {
				state = OUTSIDE;
				*s = '\0';
			}
			break;

		case SQUOTE:
			if (*s == '\'') {
				state = OUTSIDE;
				*s = '\0';
			}
			break;

		case NOQUOTE:
			if (isspace(*s)) {
				state = OUTSIDE;
				*s = '\0';
			}
			break;

		case OUTSIDE:
			if (*s == '"') {
				state = DQUOTE;
				args[argc++] = s + 1;
			} else if (*s == '\'') {
				state = SQUOTE;
				args[argc++] = s + 1;
			} else if (!isspace(*s)) {
				state = NOQUOTE;
				args[argc++] = s;
			}
			break;

		default:
			assert(!"unreachable");
			break;
		}
	}

	return argc;
}

int main(void)
{
	static char cwd_path[PATH_MAX];
	static char *args[ARGS_MAX + 1]; // One more for NULL at end

	// Use the same buffer everytime, malloc'd by getline.
	char *line_ptr = NULL;
	size_t buf_len = 0;

	if (sigsetjmp(interrupt_jmp, 1) == 0)
		signal(SIGINT, ctrlc_handler);

	while (1) {
		if (getcwd(cwd_path, sizeof cwd_path) != NULL)
			PRINTE("[%s]", cwd_path);
		else
			PRINTE("[?]");
		PRINTE("> ");

		// Re-start the prompt on a new line if interrupt recieved
		// while waiting for the command.
		if (sigsetjmp(interrupt_jmp, 1) != 0) {
			PRINTE("\n");
			continue;
		}

		ssize_t line_len = getline(&line_ptr, &buf_len, stdin);
		if (line_len == -1)
			break;

		int arg_cnt = split_into_args(line_ptr, ARGS_MAX, args);
		args[arg_cnt] = NULL;
		if (!(arg_cnt > 0 && check_if_executable(line_ptr)))
			continue;

		struct timespec start, end;
		clock_gettime(CLOCK_MONOTONIC, &start);

		// Will only run for the child process.
		if (fork() == 0) {
			// Restore Ctrl-C for the child
			signal(SIGINT, SIG_DFL);

			// Temporary workaround, pass the environment as it is.
			extern char **environ; // See man environ.7
			execve(args[0], args, environ);

			// If here then the exec syscall failed.
			perror("exec");
			free(line_ptr);
			exit(CMD_NOT_FOUND_ERR);
		}

		int status = 0;

		// Ignore interrupt in parent while the child is running.
		signal(SIGINT, SIG_IGN);
		// Wait for the child to finish.
		waitpid(-1, &status, 0);
		status = WEXITSTATUS(status);
		// Restore interrupt handler after the child has exited.
		signal(SIGINT, ctrlc_handler);

		clock_gettime(CLOCK_MONOTONIC, &end);
		double dt = timespec_diff(start, end);

		PRINTE("\nExited with %d, took %.3fs\n", status, dt);
	}

	free(line_ptr);
	PRINTE("[exit]\n");

	return 0;
}
