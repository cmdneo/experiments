/* Mathematical expression evaluator
 *
 * Grammar:
 * Precedence order: % / * + -
 * Left to right associativity for same precedence
 *
 * digit = any{0-9}
 * alpha = any{a-zA-Z}
 * non_digit = aplha | '_'
 * binop = '/' | '*' | '+' | '-'
 *
 * BINARY_FUNC := 'min' | 'max'
 * UNARY_FUNC := 'min' | 'max'
 *
 * NUMBER := digit+ ['.' digit*]
 *
 * IDENT := non_digit (non_digit | digit)*
 *
 * PAREN_EXPR := '(' EXPR ')'
 * 
 * FUNC_EXPR := BINARY_FUNC '( EXPR ',' EXPR ')'
 *            | UNARY_FUNC PAREN_EXPR
 *
 * BASE_EXPR :=
 *            | '+' BASE_EXPR
 *            | '-' BASE_EXPR
 *            | FUNC_EXPR
 *            | PAREN_EXPR
 *            | NUMBER
 *            | PARAM
 *
 * EXPR := BASE_EXPR (binop BASE_EXPR)*
 *
 */

// TODO Add support for named-parameters

#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*(arr)))

// On error we just simply print an error message and exit
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#define EPRINT(...) (DEBUG(__VA_ARGS__), exit(1))
#define PARSE_EPRINT(msg) EPRINT("ERROR before %d: %s\n", cursor, (msg))

// Common presets
enum {
	IDENT_MAX = 255,
	STACK_MAX = 512,
	CODE_MAX = 4096,
	LINE_MAX = 8192,
};

// Stack machine
//---------------------------------------------------------
typedef union Code {
	void (*fnptr)(void);
	double *val_ref;
	double val;
} Code;

// Global Stack Machine state
static Code code[CODE_MAX];
static unsigned code_cnt;
static double stack[STACK_MAX];
static unsigned stack_top;
static unsigned code_pc;

static int push_dt_code(Code cd)
{
	if (code_cnt == CODE_MAX)
		EPRINT("Expression too big\n");
	code[code_cnt++] = cd;
	return 0;
}

// Operations for stack machine
//-----------------------------------------------
#define STACK_POP_TWO(id1, id2)      \
	double id2 = stack[--stack_top]; \
	double id1 = stack[--stack_top];

void stack_push(double val)
{
	if (stack_top == STACK_MAX)
		EPRINT("VM Stack overflow\n");
	stack[stack_top++] = val;
}

static void push_value(void) { stack_push(code[code_pc++].val); }

static void push_ident(void) { stack_push(*code[code_pc++].val_ref); }

static void op_sub(void)
{
	STACK_POP_TWO(a, b);
	stack_push(a - b);
}

static void op_add(void)
{
	STACK_POP_TWO(a, b);
	stack_push(a + b);
}

static void op_mul(void)
{
	STACK_POP_TWO(a, b);
	stack_push(a * b);
}

static void op_div(void)
{
	STACK_POP_TWO(a, b);
	if (b == 0)
		EPRINT("Divide by zero\n");
	stack_push(a / b);
}

static void op_pow(void)
{
	STACK_POP_TWO(a, b);
	stack_push(pow(a, b));
}

static void op_min(void)
{
	STACK_POP_TWO(a, b);
	stack_push(a < b ? a : b);
}

static void op_max(void)
{
	STACK_POP_TWO(a, b);
	stack_push(a > b ? a : b);
}

// Generates op_<cmath_func> unary functions
#define GEN_UNR_FUNC(cmath_func)                                   \
	static void op_##cmath_func(void)                              \
	{                                                              \
		stack[stack_top - 1] = (cmath_func)(stack[stack_top - 1]); \
	}

GEN_UNR_FUNC(sin)
GEN_UNR_FUNC(cos)
GEN_UNR_FUNC(tan)
GEN_UNR_FUNC(exp)
GEN_UNR_FUNC(log)
GEN_UNR_FUNC(log10)
GEN_UNR_FUNC(log2)
GEN_UNR_FUNC(floor)
GEN_UNR_FUNC(ceil)
GEN_UNR_FUNC(round)
GEN_UNR_FUNC(sqrt)
static void op_abs(void) { stack[stack_top - 1] = fabs(stack[stack_top - 1]); }
static void op_negate(void) { stack[stack_top - 1] = -stack[stack_top - 1]; }

static void eval_code(void)
{
	code_pc = 0;
	stack_top = 0;

	while (code_pc < code_cnt)
		(*code[code_pc++].fnptr)();
}

// Parser
//---------------------------------------------------------
// Use positive values for literal characters
enum Token {
	TOK_EOF = -100,
	TOK_NUM,
	TOK_IDENT,
	TOK_BIN_FUNC,
	TOK_UNR_FUNC,
};

// clang-format off
const static char *BINOPS = "-+*/^";
static const int PRECEDENCE_TABLE[] = {
	['-'] = 1,
	['+'] = 2,
	['*'] = 3,
	['/'] = 4,
	['^'] = 5,
};

// Indexed by precedence
static void (*BINOP_FUNC_TABLE[])(void) = {
	[1] = op_sub,
	[2] = op_add,
	[3] = op_mul,
	[4] = op_div,
	[5] = op_pow,
};

static const unsigned BIN_FUNC_LAST_INDEX = 1;
static const char *FUNC_NAMES[] = {
	// Binary functions
	"min",
	"max",
	// Unary functions
	"sin",
	"cos",
	"tan",
	"exp",
	"log",
	"log10",
	"log2",
	"floor",
	"ceil",
	"round",
	"sqrt",
	"abs",
	"negate",
};

// Indexed by func_index
static void (*FUNC_TABLE[])(void) = {
	// Binary functions
	op_min,
	op_max,
	// Unary functions
	op_sin,
	op_cos,
	op_tan,
	op_exp,
	op_log,
	op_log10,
	op_log2,
	op_floor,
	op_ceil,
	op_round,
	op_sqrt,
	op_abs,
	op_negate,
};
// clang-format on

// Global Parser state
static char given_line[LINE_MAX];
static char identifier[IDENT_MAX + 1];
static double number;
static unsigned func_index;
static int cur_token;
static int cursor;
static int last_char = ' ';

static inline bool is_ident_char(int c) { return isalnum(c) || c == '_'; }

static inline bool is_binop(int c) { return strchr(BINOPS, c) != NULL; }

static inline int get_precedence(int tok)
{
	if (is_binop(tok))
		return PRECEDENCE_TABLE[tok];
	return 0;
}

static inline int my_getchar(void)
{
	if (cursor == LINE_MAX || given_line[cursor] == '\0')
		return EOF;
	return given_line[cursor++];
}

static int next_token_impl(void)
{
	// Skip blanks
	while (isblank(last_char))
		last_char = my_getchar();

	// Parse number
	if (isdigit(last_char)) {
		char numstr[IDENT_MAX + 1] = {0};
		char *tmp = numstr, *end = NULL;

		while (isdigit(last_char) || last_char == '.') {
			*tmp++ = last_char;
			if (tmp - numstr == IDENT_MAX)
				PARSE_EPRINT("Number string too long");

			number = strtod(numstr, &end);
			if (*end != '\0')
				PARSE_EPRINT("Error parsing number");

			last_char = my_getchar();
		}

		return TOK_NUM;
	}

	// Parse identifier
	if (is_ident_char(last_char)) {
		char *tmp = identifier;

		while (is_ident_char(last_char)) {
			*tmp++ = last_char;
			if (tmp - identifier == IDENT_MAX)
				PARSE_EPRINT("Parameter name too long");

			last_char = my_getchar();
		}
		*tmp = '\0';

		for (unsigned i = 0; i < ARRAY_SIZE(FUNC_NAMES); ++i) {
			if (strcmp(identifier, FUNC_NAMES[i]) == 0) {
				func_index = i;
				return i > BIN_FUNC_LAST_INDEX ? TOK_UNR_FUNC : TOK_BIN_FUNC;
			}
		}

		return TOK_IDENT;
	}

	if (last_char == EOF)
		return TOK_EOF;

	int tmp = last_char;
	last_char = my_getchar();
	return tmp;
}

static int next_token(void) { return (cur_token = next_token_impl()); }

static void parse_number(void)
{
	push_dt_code((Code){.fnptr = push_value});
	push_dt_code((Code){.val = number});
	next_token();
}

static void parse_identifier(void)
{
	push_dt_code((Code){.fnptr = push_ident});
	// push_dt_code((Code){.val_ref = });
	next_token();
}

static void parse_expr(void);

static void parse_paren_expr(void)
{
	next_token(); // Consume '('
	parse_expr();
	if (cur_token != ')')
		PARSE_EPRINT("Expected closing ')'");
	next_token();
}

static void parse_unr_func_expr(void)
{
	int fn_idx = func_index;
	next_token(); // Consume TOK_UNR_FUNC
	if (cur_token != '(')
		PARSE_EPRINT("Expected opening '('");
	parse_paren_expr();

	push_dt_code((Code){.fnptr = FUNC_TABLE[fn_idx]});
}

static void parse_bin_func_expr(void)
{
	int fn_idx = func_index;
	next_token(); // Consume TOK_BIN_FUNC
	if (cur_token != '(')
		PARSE_EPRINT("Expected opening '('");
	next_token();

	parse_expr();
	if (cur_token != ',')
		PARSE_EPRINT("Expected ','");
	next_token();

	parse_expr();
	if (cur_token != ')')
		PARSE_EPRINT("Expected closing ')'");
	next_token();

	push_dt_code((Code){.fnptr = FUNC_TABLE[fn_idx]});
}

static void parse_base_expr(void)
{
	// If a sign is at top level, then there must be a base_expr after that
	bool negated = false;
	if (cur_token == '-' || cur_token == '+') {
		negated = cur_token == '-';
		next_token(); // Eat sign
	}

	switch (cur_token) {
	case TOK_NUM:
		parse_number();
		break;

	case TOK_IDENT:
		PARSE_EPRINT("Named parameters not implemented");
		parse_identifier();
		break;

	case TOK_BIN_FUNC:
		parse_bin_func_expr();
		break;

	case TOK_UNR_FUNC:
		parse_unr_func_expr();
		break;

	case '(':
		parse_paren_expr();
		break;

	default:
		PARSE_EPRINT("Expected number or expression");
		break;
	}

	if (negated)
		push_dt_code((Code){.fnptr = op_negate});
}

// (binop BASE_EXPR)*
static void parse_binop_expr(int prev_pres)
{
	// Ambiguity resolution: prev_op BASE_EXPR cur_op BASE_EXPR next_op
	while (1) {
		int pres = get_precedence(cur_token);
		// If current binop binds less or equally tightly to the previous one,
		// then LHS expression is complete and done
		// For example: 2 + 2 * 3 - 4 is (2 + 2 * 3) - 4
		if (prev_pres >= pres)
			return;

		next_token(); // Consume binop
		parse_base_expr();

		// If next binop binds more tighly than the current one, then descend
		int next_pres = get_precedence(cur_token);
		if (next_pres > pres)
			parse_binop_expr(pres);

		push_dt_code((Code){.fnptr = BINOP_FUNC_TABLE[pres]});
	}
}

// EXPR := BASE_EXPR (binop BASE_EXPR)*
static void parse_expr(void)
{
	parse_base_expr();
	parse_binop_expr(0);
}

// Parses the line stored in given_line and generates Direct Threaded Code
static void parse_input(void)
{
	code_cnt = 0;
	cursor = 0;
	last_char = ' ';

	next_token();
	if (cur_token == '\n' || cur_token == TOK_EOF)
		return;
	else
		parse_expr();
}

//---------------------------------------------------------

int main(void)
{
	printf("========== Mathematical expression evaluator ==========\n");
	printf("Grouping supported using parenthesis\n");

	printf("Available operators:");
	for (unsigned i = 0; BINOPS[i] != '\0'; ++i)
		printf(" %c", BINOPS[i]);

	printf("\nAvailable functions:");
	for (unsigned i = 0; i < ARRAY_SIZE(FUNC_NAMES); ++i) {
		if (i % 5 == 0)
			printf("\n%20s", ""); // Indent
		printf(" %s", FUNC_NAMES[i]);
	}

	printf("\n=======================================================\n");

	while (1) {
		printf("=> ");
		if (fgets(given_line, LINE_MAX, stdin) == NULL) {
			printf("[EXIT]\n");
			return 0;
		}

		parse_input();
		eval_code();
		printf("= %.6g\n\n", stack[0]);
	}

	return 0;
}
