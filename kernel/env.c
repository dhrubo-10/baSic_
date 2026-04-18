/* baSic_ - kernel/env.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * environment variable store, simple linear key=value table
 */

#include "env.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

typedef struct {
    char key[ENV_KEY_MAX];
    char val[ENV_VAL_MAX];
    int  used;
} env_entry_t;

static env_entry_t env_table[ENV_MAX_VARS];

void env_init(void)
{
    memset(env_table, 0, sizeof(env_table));

    /* set some sensible defaults */
    env_set("OS",      "baSic_");
    env_set("VERSION", "1.0");
    env_set("AUTHOR",  "Shahriar Dhrubo");
    env_set("ARCH",    "x86");
    env_set("SHELL",   "baSic_shell");

    kprintf("[OK] env: initialized\n");
}

int env_set(const char *key, const char *val)
{
    /* update existing */
    for (int i = 0; i < ENV_MAX_VARS; i++) {
        if (env_table[i].used && !strcmp(env_table[i].key, key)) {
            strncpy(env_table[i].val, val, ENV_VAL_MAX - 1);
            env_table[i].val[ENV_VAL_MAX - 1] = '\0';
            return 1;
        }
    }
    /* insert new */
    for (int i = 0; i < ENV_MAX_VARS; i++) {
        if (!env_table[i].used) {
            strncpy(env_table[i].key, key, ENV_KEY_MAX - 1);
            strncpy(env_table[i].val, val, ENV_VAL_MAX - 1);
            env_table[i].key[ENV_KEY_MAX - 1] = '\0';
            env_table[i].val[ENV_VAL_MAX - 1] = '\0';
            env_table[i].used = 1;
            return 1;
        }
    }
    return 0;
}

const char *env_get(const char *key)
{
    for (int i = 0; i < ENV_MAX_VARS; i++)
        if (env_table[i].used && !strcmp(env_table[i].key, key))
            return env_table[i].val;
    return NULL;
}

void env_unset(const char *key)
{
    for (int i = 0; i < ENV_MAX_VARS; i++)
        if (env_table[i].used && !strcmp(env_table[i].key, key))
            env_table[i].used = 0;
}

void env_dump(void)
{
    for (int i = 0; i < ENV_MAX_VARS; i++) {
        if (!env_table[i].used) continue;
        kprintf("  %s=%s\n", env_table[i].key, env_table[i].val);
    }
}