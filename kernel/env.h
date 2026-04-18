/* baSic_ - kernel/env.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * simple key=value environment variable store
 */

#ifndef ENV_H
#define ENV_H

#define ENV_MAX_VARS    32
#define ENV_KEY_MAX     32
#define ENV_VAL_MAX     64

void        env_init(void);
int         env_set(const char *key, const char *val);
const char *env_get(const char *key);
void        env_unset(const char *key);
void        env_dump(void);     /* print all vars */

#endif