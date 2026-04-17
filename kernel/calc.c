/* baSic_ - kernel/calc.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * recursive descent parser for integer arithmetic
 * grammar:
 *   expr   = term   { ( '+' | '-' ) term   }
 *   term   = factor { ( '*' | '/' | '%' ) factor }
 *   factor = '(' expr ')' | ['-'] number
 *   number = [0-9]+
 */

#include "calc.h"
#include "../include/types.h"

static const char *pos;
static int        error;

static void skip_ws(void)
{
    while (*pos == ' ' || *pos == '\t') pos++;
}

static i32 parse_expr(void);

static i32 parse_factor(void)
{
    skip_ws();

    if (*pos == '(') {
        pos++;
        i32 val = parse_expr();
        skip_ws();
        if (*pos == ')') pos++;
        else error = 1;
        return val;
    }

    int neg = 0;
    if (*pos == '-') { neg = 1; pos++; skip_ws(); }

    if (*pos < '0' || *pos > '9') { error = 1; return 0; }

    i32 val = 0;
    while (*pos >= '0' && *pos <= '9') {
        val = val * 10 + (*pos - '0');
        pos++;
    }
    return neg ? -val : val;
}

static i32 parse_term(void)
{
    i32 val = parse_factor();
    for (;;) {
        skip_ws();
        char op = *pos;
        if (op != '*' && op != '/' && op != '%') break;
        pos++;
        i32 rhs = parse_factor();
        if (op == '*') val = val * rhs;
        else if (op == '/') {
            if (rhs == 0) { error = 1; return 0; }
            val = val / rhs;
        } else {
            if (rhs == 0) { error = 1; return 0; }
            val = val % rhs;
        }
    }
    return val;
}

static i32 parse_expr(void)
{
    i32 val = parse_term();
    for (;;) {
        skip_ws();
        char op = *pos;
        if (op != '+' && op != '-') break;
        pos++;
        i32 rhs = parse_term();
        val = (op == '+') ? val + rhs : val - rhs;
    }
    return val;
}

int calc_eval(const char *expr, i32 *out)
{
    pos   = expr;
    error = 0;
    i32 result = parse_expr();
    skip_ws();
    if (error || *pos != '\0') return 0;
    *out = result;
    return 1;
}