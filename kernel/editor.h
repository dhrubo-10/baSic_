/* baSic_ - kernel/editor.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * baSic_ text editor, edit files in ramfs
 */

#ifndef EDITOR_H
#define EDITOR_H

#define ED_ROWS      22  
#define ED_COLS      80  
#define ED_MAX_LINES 128 
#define ED_LINE_MAX  80  

/* open editor on a file path returns when user exits (Ctrl-Q) */
void editor_open(const char *path);

#endif