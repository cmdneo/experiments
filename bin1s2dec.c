#include <stdio.h>
#include <inttypes.h>
#include <assert.h>

#include <strlx/strlx.h>

strbuf *readline()
{
	return strbuf_from_file(stdin, '\n');
}


int main()
{
	long long n, b;

	strbuf *base = readline();
	strbuf *num = readline();
	
	strbuf_to_ll(base, 10, &b);
	strbuf_to_ll(num, b, &n);

	printf("NUM: %lld\n", n);

	strbuf_destroy(&base);
	strbuf_destroy(&num);
	return 0;
}
