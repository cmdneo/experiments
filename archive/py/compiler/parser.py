import re
import yaml
import pprint


def print_errors(errmsg_token_pairs, source_lines):
    for err in errmsg_token_pairs:
        print(f"Line {err[1].lineno}:")
        print(f"\x1b[31;1mERROR\x1b[0m: {err[0]}")
        print(source_lines[err[1].lineno - 1])
        print("\x1b[1;33m" + "~" * (err[1].column - 1) + "^",sep="")
        print("\x1b[0m")


class LexerError(Exception):
    pass


class ParserError(Exception):
    pass


class Config:
    TAB_WIDTH = 4
    R_PAIR_PREFIX = "R_"
    L_PAIR_PREFIX = "L_"

    TOKENS = {
        'COMMENT': r'#.*\n',
        'ID': r'[a-zA-Z_λ][a-zA-Z0-9_]*',
        'NUMBER': r'[0-9][0-9_]*(\.[0-9_]+)?((e|E)(\-|\+)?[0-9]+)?([a-zA-Z]+[a-zA-Z0-9_]*)?',
        'STRING': r'([a-zA-Z])?"(\\.|[^"])*"',
        'L_PAREN': r'\(',
        'R_PAREN': r'\)',
        'L_BRACE': r'\{',
        'R_BRACE': r'\}',
        'L_SQUARE': r'\[',
        'R_SQUARE': r'\]',
        'COMMA': r',',
        'QUOTE': r'\'',
        'LT': r'<',
        'GT': r'>',
        'EQ': r'==',
        'NEQ': r'!=',
        'LTEQ': r'<=',
        'GTEQ': r'>=',
        'LOGICAL_NOT': r'\!',
        'LOGICAL_OR': r'\|\|',
        'LOGICAL_AND': r'&&',
        'PLUS': r'\+',
        'MINUS': r'\-',
        'DIV': r'/',
        'ASTERISK': r'\*',
        'PERCENT': r'%',
        'ASSIGN': r'=',
        'AND': r'&',
        'OR': r'\|',
        'XOR': r'\^',
        'COMPL': r'~',
        'LSHIFT': r'<<',
        'RSHIFT': r'>>',
        'TO': r'->',
        'FROM': r'<-',
        'SEMICOLON': r';',
        'COLON': r':',
        'BLANK': r'\s',
        'NEWLINE_COLLAPSE': r'\\\n',
    }
    PAIRED = ["PAREN", "SQAURE", "BRACE"]
    IGNORED = ["BLANK", "COMMENT", "NEWLINE_COLLAPSE"]
    RESERVED = ["if", "else", "elif", "fn", "while", "do"]

    GRAMMAR = {
            "end": "NEWLINE | SEMICOLON",
        "literal": "NUMBER | STRING",
        "bin_op": "PLUS | MINUS | DIV | PERCENT | ASTERISK |\
LOGICAL_AND | LOGICAL_OR | LT | GT | EQ | NE | LE | GE | OR | AND | XOR | LSHIFT | RSHIFT",
        "un_op": "COMPL | LOGICAL_NOT | PLUS | MINUS",
        "value": "ID | literal | fn_call",
        "fn_call": "ID L_PAREN ((expr COMMA)* expr COMMA?)* R_PAREN",
        "bin_expr": "value bin_op value",
        "un_expr": "un_op value",
        "expr": "expr un_expr | bin_expr",
        "stmt": "ID ASSIGN expr",
        "block": "L_BRACE (block | stmt | expr)* R_BRACE",
        "fn_def": "'fn' ID L_PAREN ((ID COMMA)* ID COMMA?)* R_PAREN TO ID block",
        "if_stmt": "'if' expr block",
        "else_stmt": "'else' block",
        "elif_stmt": "'elif' expr block"
    }


class Token:
    def __init__(self, name, value, span, lineno, column):
        self.name = name
        self.value = value
        self.span = span
        self.lineno = lineno
        self.column = column

    def __repr__(self):
        return f"Token(name={self.name}, value={repr(self.value)})"


class Lexer:
    def __init__(self, config_obj, data: str):
        self.config = config_obj
        self.token_names = tuple([token for token in self.config.TOKENS])
        self.ignored = self.config.IGNORED

        # Tabs mess up debug output and are useless
        data = data.expandtabs(Config.TAB_WIDTH)
        self.data = data 
        self.lines = data.split("\n")
        self.data_len = len(data)

        self.regex = {}
        self.lineno = 1
        self.column = 1
        self.cursor = 0
        self.tokens = []
        # As [(ERROR_MESSAGE, TOKEN), ...]
        self.errors = []

        self.compile_regex()

    def update_pos_info(self, token):
        self.cursor = token.span[1]
        self.column += token.span[1] - token.span[0]

        if n := token.value.count("\n"):
            self.lineno += n
            # First column no. is 1
            self.column = len(token.value) - token.value.rfind("\n")

    def compile_regex(self):
        for k, v in self.config.TOKENS.items():
            self.regex[k] = re.compile(v)

    def lex(self):
        while self.cursor < self.data_len:
            token_matches = []

            # Collect all matching tokens
            for token_name, regex in self.regex.items():
                match = regex.match(self.data, self.cursor)
                if match:
                    token_matches.append(Token(
                        token_name,
                        match.group(),
                        match.span(),
                        self.lineno,
                        self.column
                    ))

            if not token_matches:
                self.errors.append((
                    "Illegal symbol",
                    Token("ERR", self.data[self.cursor], None, self.lineno, self.column)
                    )) 
                print_errors(self.errors, self.lines)
                raise LexerError

            # Select the longest matching token
            longest_token = max(
                    token_matches,
                    key=lambda it: len(it.value)
                    )

            self.update_pos_info(longest_token)
            if longest_token.name in self.ignored:
                continue
            self.tokens.append(longest_token)


class Parser:
    def __init__(self, tokens, paired_tokens, source_lines):
        self.tokens = tokens
        # as [(ERROR_MESSAGE, TOKEN_OBJECT)]
        self.source_lines = source_lines
        self.errors = []
        # Recursive AST: List(List(operator operand_1, operand_2, ...), ...)
        self.ast = []
        self.paired_tokens = paired_tokens

    def _is_paired_verify(self):
        pair_parities = {pt: 0 for pt in self.paired_tokens}
        token_name_pair_map = {}
        for tname, _pp, in pair_parities.items():
            token_name_pair_map[Config.L_PAIR_PREFIX + tname] = tname
            token_name_pair_map[Config.R_PAIR_PREFIX + tname] = tname

        for token in self.tokens:
            stripped_t_name = token_name_pair_map.get(token.name)
            if not stripped_t_name:
                continue

            if token.name.startswith(Config.L_PAIR_PREFIX):
                pair_parities[stripped_t_name] += 1

            if token.name.startswith(Config.R_PAIR_PREFIX):
                pair_parities[stripped_t_name] -= 1
                if pair_parities[stripped_t_name] < 0:
                    self.add_error(f"Unamtched {repr(token.value)}", token)

        for token_name, pp in pair_parities.items():
            if pp > 0:
                self.add_error(f"Unmatched {repr(tokens[-1].value)}", self.tokens[-1])

    def _fn_args_verify(self):
        pass

    def _type_safe_verify(self):
        pass

    def _eval_const_expr_pass(self):
        pass

    def add_error(self, msg, token):
        self.errors.append((msg, token))

    def gen_ast(self):
        self._is_paired_verify() 
        print_errors(self.errors, self.source_lines)
        if self.errors:
            raise ParserError

        blocks = []

        for token in self.tokens:
            if token.name == Config.BLOCK_START_SEP:
                blocks.append([])
            elif token.name == Config.BLOCK_END_SEP:
                tmp_block = blocks.pop()
                if tmp_block == []:
                    raise ParserError("Empty block")
                if not blocks:
                    self.ast.append(tmp_block)
                else:
                    blocks[-1].append(tmp_block)
            else:
                blocks[-1].append(token)


# TEST TESTING TESTING TESTING ...
with open("test.rb") as f:
    code = f.read()

lexer = Lexer(Config, code)
lexer.lex()

parser = Parser(lexer.tokens, Config.PAIRED, lexer.lines)
parser.gen_ast()

pprint.pp(parser.ast, indent=4)
