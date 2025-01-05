/* Mathematical expression evaluator
 *
 * Grammar:
 * Precedence order(high to low): ^ / * + -
 * All operators except power(^) are left associative.
 *
 * digit = any{0-9}
 * alpha = any{a-zA-Z}
 * non_digit = aplha | '_'
 * binop = '^' | '/' | '*' | '+' | '-'
 *
 * BINARY_FUNC := 'min' | 'max'
 * UNARY_FUNC := 'sin'   | 'cos'   | 'tan' | 'exp'
 *             | 'log'   | 'log10' | 'log2'
 *             | 'floor' | 'ceil'  | 'round'
 *             | 'sqrt'  | 'abs'   | 'negate'
 *
 * NUMBER := digit+ ['.' digit*] 
 *
 * PARAM := non_digit (non_digit|digit)* # Basically an identifier
 * 
 * PAREN_EXPR := '(' EXPR ')'
 * 
 * FUNC_EXPR := BINARY_FUNC '(' EXPR ',' EXPR ')'
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
 * Compile command: gcc calculator.c -lm -o calculator
 * For GNU-readline support include flags: -DREADLINE_ENABLED -lreadline
 */

#include <assert.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <setjmp.h>

#ifdef READLINE_ENABLED
#include <readline/readline.h>
#else
// Simple fallback for readline if GNU-readline not available/enabled.
char *readline(const char *prompt)
{
	const unsigned LINE_MAX = 32768;
	char *line = malloc(LINE_MAX);
	if (line == NULL)
		return NULL;

	printf("%s", prompt);
	if (fgets(line, LINE_MAX, stdin) == NULL) {
		free(line);
		return NULL;
	}

	// Remove newline
	line[strcspn(line, "\n")] = '\0';

	return line;
}
#endif // #ifdef READLINE_ENABLED

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*(arr)))

#define DEBUG(...) fprintf(stderr, __VA_ARGS__)

// On error we jump to the cleanup code and restart the calculator using this.
#define JMP_ERROR(...) \
	(DEBUG("[ERROR] "), DEBUG(__VA_ARGS__), DEBUG("\n"), longjmp(jmp_env, 1))

static jmp_buf jmp_env;

// Common presets
enum {
	PARAM_MAX = 256,
	STACK_MAX = 256,
	CODE_MAX = 4096,
};

// Stack machine
//---------------------------------------------------------
typedef void(MathFunc(void));

typedef union Code {
	MathFunc *fnptr;
	double *val_ref;
	double val;
} Code;

// Global Stack Machine state
static Code code[CODE_MAX];
static unsigned code_cnt;
static double stack[STACK_MAX];
static unsigned stack_top;
static unsigned code_pc;

// For named parameters
static char *param_names[PARAM_MAX];
static double param_values[PARAM_MAX];
static unsigned param_cnt;

static int push_dt_code(Code cd)
{
	if (code_cnt == CODE_MAX)
		JMP_ERROR("Expression too big");
	code[code_cnt++] = cd;
	return 0;
}

// Operations for the stack machine
//-----------------------------------------------
#define STACK_POP_TWO(id1, id2) \
	double id2 = stack_pop();   \
	double id1 = stack_pop();

static void stack_push(double val)
{
	if (stack_top == STACK_MAX)
		JMP_ERROR("VM Stack overflow\n");
	stack[stack_top++] = val;
}

static double stack_pop(void)
{
	if (stack_top == 0)
		JMP_ERROR("Pop on empty stack, this should never happen\n");
	return stack[--stack_top];
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
		JMP_ERROR("Divide by zero");
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

static void op_atan2(void)
{
	STACK_POP_TWO(a, b);
	stack_push(atan2(a, b));
}

// Generates unary functions
#define GEN_UNARY_FN(gen_name, cmath_func)                         \
	static void gen_name(void)                                     \
	{                                                              \
		stack[stack_top - 1] = (cmath_func)(stack[stack_top - 1]); \
	}

GEN_UNARY_FN(op_sin, sin)
GEN_UNARY_FN(op_cos, cos)
GEN_UNARY_FN(op_tan, tan)
GEN_UNARY_FN(op_asin, asin)
GEN_UNARY_FN(op_acos, acos)
GEN_UNARY_FN(op_atan, atan)
GEN_UNARY_FN(op_sinh, sinh)
GEN_UNARY_FN(op_cosh, cosh)
GEN_UNARY_FN(op_tanh, tanh)
GEN_UNARY_FN(op_asinh, asinh)
GEN_UNARY_FN(op_acosh, acosh)
GEN_UNARY_FN(op_atanh, atanh)
GEN_UNARY_FN(op_exp, exp)
GEN_UNARY_FN(op_log, log)
GEN_UNARY_FN(op_log10, log10)
GEN_UNARY_FN(op_log2, log2)
GEN_UNARY_FN(op_floor, floor)
GEN_UNARY_FN(op_ceil, ceil)
GEN_UNARY_FN(op_round, round)
GEN_UNARY_FN(op_sqrt, sqrt)
GEN_UNARY_FN(op_abs, fabs)

#undef GEN_UNARY_FN

static void op_negate(void) { stack[stack_top - 1] = -stack[stack_top - 1]; }

static void execute_code(void)
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
	TOK_PARAM,
	TOK_BIN_FUNC,
	TOK_UNR_FUNC,
};

// Simple and general Pratt parsing, refer to:
// https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html
// Left and right precedence(binding power) for associativity.
typedef struct Precedence {
	short left;
	short right;
} Precedence;

typedef struct FuncNamePair {
	int arity;
	const char *name;
	MathFunc *fn;
} FuncNamePair;

// clang-format off
static const char BINOPS[] = "-+*/^";
static const Precedence PRECEDENCE_TABLE[] = {
	['-'] = {10, 11},
	['+'] = {20, 21},
	['*'] = {30, 31},
	['/'] = {40, 41},
	['^'] = {51, 50},
};

static MathFunc *const BINOP_FUNC_TABLE[] = {
	['-'] = op_sub,
	['+'] = op_add,
	['*'] = op_mul,
	['/'] = op_div,
	['^'] = op_pow,
};

#define FP(arity, name) {arity, #name, op_##name}

static const FuncNamePair FUNC_NAME_PAIRS[] = {
	FP(2, min),
	FP(2, max),
	FP(1, sin),
	FP(1, cos),
	FP(1, tan),
	FP(1, asin),
	FP(1, acos),
	FP(1, atan),
	FP(2, atan2),
	FP(1, sinh),
	FP(1, cosh),
	FP(1, tanh),
	FP(1, asinh),
	FP(1, acosh),
	FP(1, atanh),
	FP(1, exp),
	FP(1, log),
	FP(1, log10),
	FP(1, log2),
	FP(1, floor),
	FP(1, ceil),
	FP(1, round),
	FP(1, sqrt),
	FP(1, abs),
	FP(1, negate),
};

#undef FP
// clang-format on

// Global parser state
static char *given_line;
static char *identifier; // Refers to strings in given_line
static double number; // Parsed numeric for literal
static MathFunc *math_fnptr; // Current function/operator
static unsigned param_index; // Named parameter index

// Global lexer state
static int cur_token;
static int cursor;
static int last_char = ' ';

static inline bool is_ident_char(int c) { return isalnum(c) || c == '_'; }

static inline bool is_binop(int c) { return strchr(BINOPS, c) != NULL; }

static inline Precedence get_precedence(int token)
{
	if (is_binop(token))
		return PRECEDENCE_TABLE[token];
	return (Precedence){0};
}

static inline MathFunc *get_binop_function(int token)
{
	if (is_binop(token))
		return BINOP_FUNC_TABLE[token];
	assert(!"Unreachable");
	return NULL;
}

static inline int my_getchar(void)
{
	if (given_line[cursor] == '\0') {
		cursor++;
		return EOF;
	}
	return given_line[cursor++];
}

static int next_token_impl(void)
{
	// Skip blanks
	while (isblank(last_char))
		last_char = my_getchar();

	// Parse number
	if (isdigit(last_char)) {
		char numstr[256] = {0};
		char *tmp = numstr, *end = NULL;

		while (isdigit(last_char) || last_char == '.') {
			*tmp++ = last_char;
			if (tmp - numstr == ARRAY_SIZE(numstr))
				JMP_ERROR("Number string too long");

			number = strtod(numstr, &end);
			if (*end != '\0')
				JMP_ERROR("Error parsing number");

			last_char = my_getchar();
		}

		return TOK_NUM;
	}

	// Parse identifier
	if (is_ident_char(last_char)) {
		identifier = &given_line[cursor - 1];
		while (is_ident_char(last_char))
			last_char = my_getchar();

		// Insert NULL terminator where the last char was read
		given_line[cursor - 1] = '\0';

		// Check if is a function name
		for (unsigned i = 0; i < ARRAY_SIZE(FUNC_NAME_PAIRS); ++i) {
			FuncNamePair tmp = FUNC_NAME_PAIRS[i];

			if (strcmp(identifier, tmp.name) == 0) {
				math_fnptr = tmp.fn;
				return tmp.arity == 1 ? TOK_UNR_FUNC : TOK_BIN_FUNC;
			}
		}

		// Check if parameter name already exists, if not, then insert a new one
		for (unsigned i = 0; i < param_cnt; ++i) {
			if (strcmp(identifier, param_names[i]) == 0) {
				param_index = i;
				return TOK_PARAM;
			}
		}
		if (param_cnt == PARAM_MAX)
			JMP_ERROR("Too many parameter, max allowed is %d", PARAM_MAX);
		param_index = param_cnt;
		param_names[param_cnt++] = identifier;

		return TOK_PARAM;
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
	next_token(); // Consume TOK_NUM
}

static void parse_parameter(void)
{
	push_dt_code((Code){.fnptr = push_ident});
	push_dt_code((Code){.val_ref = &param_values[param_index]});
	next_token(); // Comsume TOK_PARAM
}

static void parse_expr(void);

static void parse_paren_expr(void)
{

	next_token(); // Consume '('
	parse_expr();
	if (cur_token != ')')
		JMP_ERROR("Expected closing ')'");
	next_token();
}

static void parse_unr_func_expr(void)
{

	MathFunc *fnptr = math_fnptr;
	next_token(); // Consume TOK_UNR_FUNC
	if (cur_token != '(')
		JMP_ERROR("Expected opening '('");
	parse_paren_expr();

	push_dt_code((Code){.fnptr = fnptr});
}

static void parse_bin_func_expr(void)
{
	MathFunc *fnptr = math_fnptr;
	next_token(); // Consume TOK_BIN_FUNC
	if (cur_token != '(')
		JMP_ERROR("Expected opening '('");
	next_token();

	parse_expr();
	if (cur_token != ',')
		JMP_ERROR("Expected ','");
	next_token();

	parse_expr();
	if (cur_token != ')')
		JMP_ERROR("Expected closing ')'");
	next_token();

	push_dt_code((Code){.fnptr = fnptr});
}

static void parse_base_expr(void)
{
	// Handle if a sign before (maybe)base_expr
	if (cur_token == '-' || cur_token == '+') {
		bool negated = cur_token == '-';
		next_token(); // Eat sign
		parse_base_expr();
		if (negated)
			push_dt_code((Code){.fnptr = op_negate});
		return;
	}

	switch (cur_token) {
	case TOK_NUM:
		parse_number();
		break;

	case TOK_PARAM:
		parse_parameter();
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
		JMP_ERROR("Expected a number or expression");
		break;
	}
}

// (binop BASE_EXPR)*
static void parse_binop_expr(Precedence prev_pres)
{
	// Ambiguity resolution: prev_op BASE_EXPR cur_op BASE_EXPR next_op
	// We handle associativity requirements using different value for the
	// left and right precedence of the same operator.
	while (1) {
		Precedence pres = get_precedence(cur_token);
		int binop_tok = cur_token;
		// If current binop binds less tightly than the previous one then, the
		// previous binop has higher precedence or the both binops are same
		// and have left-associativity.
		if (prev_pres.right >= pres.left)
			return;

		next_token(); // Consume binop
		parse_base_expr();

		// If next binop binds more tighly than the current one then, the
		// next binop has higher precedence or the both binops are same
		// and have right-associativity.
		Precedence next_pres = get_precedence(cur_token);
		if (pres.right < next_pres.left)
			parse_binop_expr(pres);

		push_dt_code((Code){.fnptr = get_binop_function(binop_tok)});
	}
}

// EXPR := BASE_EXPR (binop BASE_EXPR)*
static void parse_expr(void)
{
	parse_base_expr();
	parse_binop_expr((Precedence){0, 0});
}

// Parses the line stored in given_line and generates Direct Threaded Code.
// Returns false on empty input, true if parsed successfully.
static bool parse_input(void)
{
	param_cnt = 0;
	code_cnt = 0;
	cursor = 0;
	last_char = ' ';

	next_token();

	// Do nothing on empty line.
	if (cur_token == '\n' || cur_token == '\r' || cur_token == TOK_EOF)
		return false;
	else
		parse_expr();

	// Make sure that the input has been fully parsed.
	// That is: expr NEWLINE | expr NEWLINE
	if (cur_token != TOK_EOF && cur_token != '\n')
		JMP_ERROR("Invalid token sequence in expression");

	return true;
}

static void input_param_values(void)
{
	if (param_cnt == 0)
		return;

	// Prompt for readline as just printing the prompt using printf does not
	// work, because when the line is cleared by readline it clears that too
	char prompt[1024];
	printf("Input values for:\n");
	for (unsigned i = 0; i < param_cnt; ++i) {
		int pmax = ARRAY_SIZE(prompt) - 24;
		snprintf(prompt, ARRAY_SIZE(prompt), "> %.*s = ", pmax, param_names[i]);

		char *line = readline(prompt);
		if (line == NULL)
			JMP_ERROR("Cannot read number");

		char end = 0;
		// Check that number has no invalid characters at end.
		if (sscanf(line, "%lf %c", &param_values[i], &end) != 1) {
			free(line);
			JMP_ERROR("Invalid number");
		}

		free(line);
	}
}

//---------------------------------------------------------

int main(void)
{
#ifdef READLINE_ENABLED
	rl_bind_key('\t', rl_insert); // Disable TAB autocomplete
#endif

	// Print help text
	//-----------------------------------------------------------------
	printf("========== Mathematical expression evaluator ==========\n");
	printf("Grouping using parenthesis and\n"
		   "named parameters are supported\n\n");

	printf("Available operators:");
	for (unsigned i = 0; BINOPS[i] != '\0'; ++i)
		printf(" %c", BINOPS[i]);

	printf("\nAvailable functions:");
	for (unsigned i = 0; i < ARRAY_SIZE(FUNC_NAME_PAIRS); ++i) {
		if (i % 5 == 0)
			printf("\n%20s", ""); // Indent
		printf(" %s", FUNC_NAME_PAIRS[i].name);
	}

	printf("\n=======================================================\n");

	while (1) {
		given_line = readline("=> ");
		if (given_line == NULL) {
			printf("[EXIT]\n");
			return 0;
		}

		// For error recovery and restart.
		if (setjmp(jmp_env) != 0) {
			// Error jmp, do nothing.
		} else if (parse_input()) {
			input_param_values();
			execute_code();
			printf("= %g\n", stack_pop());
		}

		free(given_line);
		given_line = NULL;
	}

	return 0;
}
