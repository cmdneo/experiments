#include <stdio.h>

typedef struct bitf_3 {
	unsigned e1 : 1;
	unsigned e2 : 1;
	unsigned e3 : 1;
} bitf_3;

int main() {
	bitf_3 bf3 = { 0 };
	__builtin_dump_struct(&bf3, &printf);
}
