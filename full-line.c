#include <stdio.h>
#include <strlx/strlx.h>

strbuf* readline() {
	return strbuf_from_file(stdin, '\n');
}

int main() {
	strbuf *line = readline();
	strbuf_print(line);

	puts("");
	strbuf_destroy(&line);

}
