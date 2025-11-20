#include "userlib.h"
#include <stdarg.h>

#ifdef HOSTED

void u_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

int u_ls(const char *path) {
    const char *p = path ? path : ".";
    DIR *d = opendir(p);
    if (!d) { perror("opendir"); return -1; }
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        printf("%s  ", entry->d_name);
    }
    printf("\n");
    closedir(d);
    return 0;
}

int u_cat(const char *path) {
    if (!path) { printf("cat: missing file\n"); return -1; }
    FILE *f = fopen(path, "r");
    if (!f) { perror("fopen"); return -1; }
    int c;
    while ((c = fgetc(f)) != EOF) putchar(c);
    fclose(f);
    return 0;
}

int u_echo(const char *s) {
    if (s) puts(s);
    else puts("");
    return 0;
}

int u_edit(const char *filename) {
    /* Simple hosted fallback: use system $EDITOR if available, else edit with nano */
    const char *editor = getenv("EDITOR");
    if (!editor) editor = "nano";
    char cmd[512];
    if (filename) snprintf(cmd, sizeof(cmd), "%s %s", editor, filename);
    else snprintf(cmd, sizeof(cmd), "%s", editor);
    return system(cmd);
}

int u_ps(void) {
    printf("PID   NAME\n");
    system("ps -ef | sed -n '1,10p'"); /* quick hosted view */
    return 0;
}

void u_run_init(void) {
    /* On hosted, run the shell executable compiled here */
    extern int main_sh(int argc, char **argv);
    char *args[] = { "sh", NULL };
    main_sh(1, args);
    /* returned -> exit */
}

#else

#include "../f_sys/fs.h"
#include "../inc/shell.h"
#include "../inc/editor.h"
#include "../inc/terminal.h"
#include "../inc/system.h"
#include "../mm/kmalloc.h"
#include "../inc/string.h"
#include "../inc/tools.h"

void u_printf(const char *fmt, ...) {
    /* minimal printf-like: only supports %s and %d (for kernel use) */
    va_list ap;
    va_start(ap, fmt);
    const char *p = fmt;
    char numbuf[32];
    while (*p) {
        if (*p == '%') {
            p++;
            if (*p == 's') {
                char *s = va_arg(ap, char*);
                term_puts(s ? s : "(null)");
            } else if (*p == 'd') {
                int v = va_arg(ap, int);
                itoa(v, numbuf, 10);
                term_puts(numbuf);
            } else {
                term_putc('%'); term_putc(*p);
            }
            p++;
        } else {
            term_putc(*p++);
        }
    }
    va_end(ap);
}

/* In freestanding mode we use your in-memory FS functions */
int u_ls(const char *path) {
    (void)path;
    fs_list(current_dir);
    return 0;
}

int u_cat(const char *path) {
    if (!path) { u_printf("cat: missing file\n"); return -1; }
    fs_read(path);
    return 0;
}

int u_echo(const char *s) {
    if (s) term_puts(s);
    term_puts("\n");
    return 0;
}

int u_edit(const char *filename) {
    if (filename) editor_open(filename);
    else editor_open("untitled");
    return 0;
}

int u_ps(void) {
    /* stub — if you implement proc table, print it here */
    term_puts("PID   NAME\n");
    term_puts("1     init\n");
    return 0;
}

void u_run_init(void) {
    /* In kernel, call the shell main directly */
    shell_main();
}

#endif
