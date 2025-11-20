#ifndef USERLIB_H
#define USERLIB_H

#ifdef HOSTED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#else
#include "../include/kernel_b/types.h"
#include "../f_sys/fs.h"     
#include "../inc/editor.h"
#include "../inc/system.h"
#include "../inc/terminal.h"
#endif

/* portable print */
void u_printf(const char *fmt, ...);

/* filesystem helpers */
int u_ls(const char *path);     /* returns 0 on success */
int u_cat(const char *path);    /* prints file */
int u_echo(const char *s);

/* editor entry (opens file if name != NULL, else empty buffer) */
int u_edit(const char *filename);

/* process list / ps (stub) */
int u_ps(void);

/* init / run shell (for kernel this may call shell_main) */
void u_run_init(void);

#endif
