R"""
Calculates roots of a simple 2 degree equation, grouping or braces not
supported. Single variable only. Using them will lead to UNEXPECTED results
or crash or can set your computer on fire.

Author: Amiy K

LICENSE
Permission is hereby granted to do whatever you want to do with this
computer program on the conditions that:
1 => You cannot hold me liable due to any kind damage caused by this program
     under any circumstances.
"""


import operator
import re
import sys
from functools import partial
from dataclasses import dataclass


@dataclass
class Token:
    val: str
    typ: str


@dataclass
class Term:
    coeff: float
    sym: str
    deg: int


tok_res = {
    "dig": re.compile(r'[0-9]+\.?([0-9]+)?'),
    "sym": re.compile(r'[a-zA-Z](_\w+)?'),
    "opr": re.compile(r'[-+*/^=]'),
    "not_impl": re.compile(r'[)([\]{})]')
}

float_re = re.compile(r'[0-9]+\.')


def parse_polyn(s: str) -> list[Term]:
    s = re.sub(r'\s+', "", s)
    tokens: list[Token] = []
    prev: Token = None

    while s:
        for typ, rgx in tok_res.items():
            match = rgx.match(s)
            if match is None:
                continue

            tok = Token(match.group(), typ)
            tokens.append(tok)
            if tok.typ == "not_impl":
                raise NotImplementedError

            s = s[match.span()[1]:]

            if prev is None:
                prev = tok
                continue

            # If two consecutive: sym & sym, sym & dig, dig & sym.
            # Then insert a mul operator in between
            if (tok.typ == "dig" and prev.typ == "sym") \
                    or (tok.typ == "sym" and prev.typ == "dig") \
                    or (tok.typ == "sym" and prev.typ == "sym"):
                tokens.append(Token("*", "opr"))

            # If two consecutive: dig & dig, opr & opr. Then error
            if (tok.typ == "dig" and prev.typ == "dig") \
                    or (tok.typ == "opr" and prev.typ == "opr"):
                raise ValueError("Absent operand/operator between tokens")

            prev = tok

    if len(tokens) == 0:
        raise ValueError("Empty equation")

    if tokens[-1].typ == "opr":
        raise ValueError("Stray operator at the end")

    if len(tokens) >= 2 \
            and tokens[0].typ == "opr" and tokens[0].val not in "+-" \
            and tokens[1].typ == "dig":
        raise ValueError("Non-unary operator at start")

    if tokens.count(Token("=", "opr")) != 1:
        raise ValueError("Invalid equation, extra or no equal sign")

    # Add sign (as opr token) if not present to the terms. makes parsing easy
    if tokens[0].typ != "opr":
        tokens.insert(0, Token("+", "opr"))

    eq_at = tokens.index(Token("=", "opr"))
    if tokens[eq_at + 1].typ != "opr":
        tokens.insert(eq_at + 1, Token("+", "opr"))

    terms: Term = []
    sign = 1
    deg_sign = 1
    fn = operator.mul
    prev_opd = "dig"

    # Generate the exp terms
    for tok in tokens:
        if tok == Token("+", "opr"):
            terms.append(Term(1 * sign, "", 0))
            fn = operator.mul
        elif tok == Token("-", "opr"):
            terms.append(Term(-1 * sign, "", 0))
            fn = operator.mul

        elif tok == Token("*", "opr"):
            fn = operator.mul
        elif tok == Token("/", "opr"):
            fn = operator.truediv
        elif tok == Token("^", "opr"):
            fn = operator.pow

        # Flip sign after equal detetcted
        elif tok == Token("=", "opr"):
            sign = -1
            prev_opd = "nil"

        elif tok.typ == "sym":
            if fn == operator.truediv:
                deg_sign = -1
            else:
                deg_sign = 1

            terms[-1].sym = tok.val
            terms[-1].deg += 1 * deg_sign
            prev_opd = "sym"

        # If prev operand was a sym and opr is ^(pow) then add to the degree
        elif fn == operator.pow and prev_opd == "sym" and tok.typ == "dig":
            if float_re.match(tok.val) or int(tok.val) < 0:
                raise ValueError("Degree can only be non-negative integer")
            terms[-1].deg += deg_sign * (int(tok.val) - 1)

        elif tok.typ == "dig":
            terms[-1].coeff = fn(terms[-1].coeff, float(tok.val))
            prev_opd = "dig"

    terms = sorted(terms, key=lambda x: x.deg, reverse=True)

    # import pprint
    # pprint.pp(terms)

    return terms


def solve_quad(s: str):
    terms = parse_polyn(s)
    a = b = c = 0

    if terms[0].deg != 2:
        raise ValueError("Needs a quadratic(degree two) equation")

    for d2 in filter(lambda x: x.deg == 2, terms):
        a += d2.coeff
    for d1 in filter(lambda x: x.deg == 1, terms):
        b += d1.coeff
    for d0 in filter(lambda x: x.deg == 0, terms):
        c += d0.coeff

    d = b ** 2 - 4 * a * c
    r1 = (-b + d ** 0.5) / (2 * a)
    r2 = (-b - d ** 0.5) / (2 * a)

    return (r1, r2, terms[0].sym)


# Demo, may work, may not work ;)
perr = partial(print, file=sys.stderr)
try:
    r1, r2, sym = solve_quad(input("Input equation of degree 2\n=> "))
except ValueError as e:
    perr("Error:", e)
    sys.exit(1)
except NotImplementedError:
    perr("Functionality you are trying to use is not present.")
    sys.exit(1)


print(f"{sym}_1 = {r1:.3f}\n{sym}_2 = {r2:.3f}")
