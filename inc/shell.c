// kernel/shell.c
// Advanced shell: builtins + hosted external execution + simple pipes/redir
#include "terminal.h"
#include "tools.h"
#include "system.h"
#include "string.h"
#include "../fs/fs.h"   /* optional: use when FREESTANDING */
#include <stdlib.h>
#include <stdbool.h>

#ifdef HOSTED
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif

#define MAX_LINE 512
#define MAX_ARGS 64

/* forward */
static void run_builtin(char **argv);
static bool is_builtin(const char *cmd);

/* simple tokenizer (splits by whitespace; does not handle quotes) */
static int tokenize(char *line, char **argv, int maxargs) {
    int argc = 0;
    char *p = line;
    while (*p && argc < maxargs) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) { *p = '\0'; p++; }
    }
    argv[argc] = NULL;
    return argc;
}

/* parse for a single '|' pipe dividing two commands.
   returns index of pipe arg or -1 if none. */
static int find_pipe(char **argv) {
    for (int i = 0; argv[i]; ++i) {
        if (strcmp(argv[i], "|") == 0) return i;
    }
    return -1;
}

/* parse redirection operators > and <. returns file name in out/in params */
static void handle_redirections(char **argv, char **outfile, char **infile, bool *background) {
    *outfile = NULL; *infile = NULL; *background = false;
    for (int i = 0; argv[i]; ++i) {
        if (strcmp(argv[i], ">") == 0 && argv[i+1]) {
            *outfile = argv[i+1];
            argv[i] = NULL;
            argv[i+1] = NULL;
            break;
        }
        if (strcmp(argv[i], "<") == 0 && argv[i+1]) {
            *infile = argv[i+1];
            argv[i] = NULL;
            argv[i+1] = NULL;
            break;
        }
        if (strcmp(argv[i], "&") == 0) {
            *background = true;
            argv[i] = NULL;
            break;
        }
    }
}

/* Run a command in HOSTED mode (supports pipes & redir) */
#ifdef HOSTED
static void run_command_hosted(char **argv) {
    if (!argv[0]) return;
    if (is_builtin(argv[0])) {
        run_builtin(argv);
        return;
    }

    /* check for pipe */
    int pipe_idx = find_pipe(argv);
    if (pipe_idx >= 0) {
        argv[pipe_idx] = NULL;
        char **left = argv;
        char **right = argv + pipe_idx + 1;
        int fdpipe[2];
        if (pipe(fdpipe) < 0) { perror("pipe"); return; }
        pid_t pid1 = fork();
        if (pid1 == 0) {
            /* left child -> writes to pipe */
            dup2(fdpipe[1], STDOUT_FILENO);
            close(fdpipe[0]); close(fdpipe[1]);
            execvp(left[0], left);
            perror("exec");
            _exit(127);
        }
        pid_t pid2 = fork();
        if (pid2 == 0) {
            /* right child -> reads from pipe */
            dup2(fdpipe[0], STDIN_FILENO);
            close(fdpipe[0]); close(fdpipe[1]);
            execvp(right[0], right);
            perror("exec");
            _exit(127);
        }
        close(fdpipe[0]); close(fdpipe[1]);
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
        return;
    }

    /* no pipe: simple redirection/background handling */
    char *outfile=NULL, *infile=NULL;
    bool background=false;
    handle_redirections(argv, &outfile, &infile, &background);

    pid_t pid = fork();
    if (pid == 0) {
        if (outfile) {
            int fd = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0) { perror("open"); _exit(127); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if (infile) {
            int fd = open(infile, O_RDONLY);
            if (fd < 0) { perror("open"); _exit(127); }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        execvp(argv[0], argv);
        perror("exec");
        _exit(127);
    } else if (pid > 0) {
        if (!background) waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
}
#endif

/* Run a command in FREESTANDING mode (builtins only) */
static void run_command_freestanding(char **argv) {
    if (!argv[0]) return;
    if (is_builtin(argv[0])) {
        run_builtin(argv);
    } else {
        term_puts("Unknown command (freestanding): ");
        term_puts(argv[0]);
        term_puts("\n");
    }
}

/* Shell main loop */
void shell_main(void) {
    char line[MAX_LINE];
    char *argv[MAX_ARGS];

    term_puts("Welcome to baSic_ shell (advanced)\n");
    while (1) {
        term_prompt("baSic_");
        /* read input from terminal layer */
#ifdef HOSTED
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = '\0';
#else
        term_input(line, sizeof(line)); /* term_input implemented in terminal.c for freestanding */
#endif
        /* empty */
        if (!line[0]) continue;

        /* quick split by '|' left-right (tokenize) */
        int argc = tokenize(line, argv, MAX_ARGS);
        if (argc == 0) continue;

#ifdef HOSTED
        run_command_hosted(argv);
#else
        run_command_freestanding(argv);
#endif
    }
}

/* ---- builtins ---- */
/* minimal builtins: echo, clear, ls, time, uptime, help, reboot, shutdown, cat, mkdir, rmdir, touch, rm, cd, pwd */
static void run_builtin(char **argv) {
    const char *cmd = argv[0];
    if (strcmp(cmd, "help")==0) {
        sys_help();
    } else if (strcmp(cmd, "clear")==0) {
        tool_clear();
    } else if (strcmp(cmd, "echo")==0) {
        if (argv[1]) tool_echo(argv[1]);
        else tool_echo("");
    } else if (strcmp(cmd, "ls")==0) {
#ifdef HOSTED
        tool_ls();
#else
        fs_list(current_dir);
#endif
    } else if (strcmp(cmd, "time")==0) {
        char buf[64];
#ifdef HOSTED
        clock_now(buf, sizeof(buf));
        term_puts(buf); term_puts("\n");
#else
        clock_print_time();
#endif
    } else if (strcmp(cmd, "uptime")==0) {
        tool_uptime();
    } else if (strcmp(cmd, "reboot")==0) {
        system_reboot();
    } else if (strcmp(cmd, "shutdown")==0 || strcmp(cmd, "halt")==0) {
        system_halt();
    } else if (strcmp(cmd, "cat")==0) {
        if (!argv[1]) { term_puts("Usage: cat <file>\n"); return; }
#ifdef HOSTED
        /* hosted: use system cat */
        char cmdline[512]; snprintf(cmdline, sizeof(cmdline), "cat %s", argv[1]); system(cmdline);
#else
        fs_read(argv[1]);
#endif
    } else if (strcmp(cmd, "mkdir")==0) {
        if (!argv[1]) { term_puts("Usage: mkdir <name>\n"); return; }
#ifdef HOSTED
        char cmdline[512]; snprintf(cmdline, sizeof(cmdline), "mkdir -p %s", argv[1]); system(cmdline);
#else
        fs_mkdir(argv[1]);
#endif
    } else if (strcmp(cmd, "rmdir")==0) {
#ifdef HOSTED
        char cmdline[512]; snprintf(cmdline, sizeof(cmdline), "rmdir %s", argv[1]); system(cmdline);
#else
        fs_rmdir(argv[1]);
#endif
    } else if (strcmp(cmd, "touch")==0) {
#ifdef HOSTED
        char cmdline[512]; snprintf(cmdline, sizeof(cmdline), "touch %s", argv[1]); system(cmdline);
#else
        fs_touch(argv[1]);
#endif
    } else if (strcmp(cmd, "rm")==0) {
#ifdef HOSTED
        char cmdline[512]; snprintf(cmdline, sizeof(cmdline), "rm -f %s", argv[1]); system(cmdline);
#else
        fs_rm(argv[1]);
#endif
    } else if (strcmp(cmd, "cd")==0) {
        if (!argv[1]) { term_puts("Usage: cd <dir>\n"); return; }
#ifdef HOSTED
        if (chdir(argv[1]) != 0) { term_puts("chdir failed\n"); }
#else
        fs_cd(argv[1]);
#endif
    } else if (strcmp(cmd, "pwd")==0) {
#ifdef HOSTED
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd))) { term_puts(cwd); term_puts("\n"); }
#else
        term_puts(current_dir->name); term_puts("\n");
#endif
    } else {
        term_puts("builtin not implemented: ");
        term_puts(cmd);
        term_puts("\n");
    }
}

static bool is_builtin(const char *cmd) {
    const char *list[] = {
        "help","clear","echo","ls","time","uptime","reboot","shutdown","halt",
        "cat","mkdir","rmdir","touch","rm","cd","pwd", NULL
    };
    for (int i = 0; list[i]; ++i) if (strcmp(cmd, list[i])==0) return true;
    return false;
}
