/* baSic_ - kernel/calc.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * integer expression evaluator: +  -  *  /  %  ( )
 */

#ifndef CALC_H
#define CALC_H

#include "../include/types.h"

/* evaluates expression string, writes result to *out */
int calc_eval(const char *expr, i32 *out);

#endif