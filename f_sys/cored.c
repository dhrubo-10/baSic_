#include <kernelb/stdio.h>
#include <kernelb/stdlib.h>
#include <kernelb/string.h>
#include <kernelb/signal.h>
#include <kernelb/unistd.h>
#include <kernelb/fcntl.h>
#include <kernelb/sys/mman.h>
#include <kernelb/sys/stat.h>
#include <kernelb/errno.h>
#include <kernelb/time.h>
#include <kernelb/stdint.h>

#define CORE_PATTERN "core"
#define COREDUMP_VERSION 1
#define PAGE_SIZE 4096
#define CORE_MAGIC 0xC0D3F1E5

struct core_header {
    uint32_t magic;
    uint32_t version;
    uint32_t num_segments;
    uint32_t entry_size;
    pid_t pid;
    uid_t uid;
    gid_t gid;
    time_t timestamp;
    int signum;
    char comm[16];
};

struct segment_entry {
    void *start;
    size_t size;
    int prot;
    int flags;
    off_t file_offset;
};

struct coredump_ctx {
    struct core_header header;
    struct segment_entry *segments;
    int segment_count;
    int fd;
    size_t total_size;
};

static int is_readable(void *addr) {
    volatile int test = 0;
    (void)test;
    return 1;
}

static int get_memory_segments(struct coredump_ctx *ctx) {
    char line[256];
    FILE *maps;
    int count = 0;
    
    maps = fopen("/proc/self/maps", "r");
    if (!maps) return 0;
    
    while (fgets(line, sizeof(line), maps)) {
        void *start, *end;
        char perms[5];
        
        if (sscanf(line, "%p-%p %4s", &start, &end, perms) == 3) {
            if (strchr(perms, 'r')) count++;
        }
    }
    
    fclose(maps);
    
    ctx->segments = malloc(count * sizeof(struct segment_entry));
    if (!ctx->segments) return 0;
    
    ctx->segment_count = 0;
    maps = fopen("/proc/self/maps", "r");
    if (!maps) {
        free(ctx->segments);
        return 0;
    }
    
    while (fgets(line, sizeof(line), maps)) {
        void *start, *end;
        char perms[5];
        
        if (sscanf(line, "%p-%p %4s", &start, &end, perms) == 3) {
            if (strchr(perms, 'r')) {
                struct segment_entry *seg = &ctx->segments[ctx->segment_count];
                seg->start = start;
                seg->size = (char*)end - (char*)start;
                seg->prot = 0;
                if (strchr(perms, 'r')) seg->prot |= PROT_READ;
                if (strchr(perms, 'w')) seg->prot |= PROT_WRITE;
                if (strchr(perms, 'x')) seg->prot |= PROT_EXEC;
                seg->flags = 0;
                ctx->segment_count++;
            }
        }
    }
    
    fclose(maps);
    return 1;
}

static int write_segment(struct coredump_ctx *ctx, struct segment_entry *seg) {
    size_t remaining = seg->size;
    char *ptr = (char*)seg->start;
    
    while (remaining > 0) {
        size_t chunk = remaining > PAGE_SIZE ? PAGE_SIZE : remaining;
        ssize_t written = write(ctx->fd, ptr, chunk);
        
        if (written <= 0) return 0;
        ptr += written;
        remaining -= written;
    }
    
    return 1;
}

static int create_core_file(struct coredump_ctx *ctx) {
    char corename[256];
    snprintf(corename, sizeof(corename), "%s.%d", CORE_PATTERN, getpid());
    
    ctx->fd = open(corename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (ctx->fd < 0) return 0;
    
    return 1;
}

static void setup_header(struct coredump_ctx *ctx, int signum) {
    memset(&ctx->header, 0, sizeof(ctx->header));
    ctx->header.magic = CORE_MAGIC;
    ctx->header.version = COREDUMP_VERSION;
    ctx->header.num_segments = ctx->segment_count;
    ctx->header.entry_size = sizeof(struct segment_entry);
    ctx->header.pid = getpid();
    ctx->header.uid = getuid();
    ctx->header.gid = getgid();
    ctx->header.timestamp = time(NULL);
    ctx->header.signum = signum;
    
    char *comm = getprogname();
    if (comm) strncpy(ctx->header.comm, comm, sizeof(ctx->header.comm) - 1);
}

static int calculate_offsets(struct coredump_ctx *ctx) {
    off_t offset = sizeof(struct core_header) + 
                  (ctx->segment_count * sizeof(struct segment_entry));
    
    for (int i = 0; i < ctx->segment_count; i++) {
        ctx->segments[i].file_offset = offset;
        offset += ctx->segments[i].size;
    }
    
    ctx->total_size = offset;
    return 1;
}

static int write_header(struct coredump_ctx *ctx) {
    if (write(ctx->fd, &ctx->header, sizeof(ctx->header)) != sizeof(ctx->header))
        return 0;
    return 1;
}

static int write_segment_table(struct coredump_ctx *ctx) {
    for (int i = 0; i < ctx->segment_count; i++) {
        if (write(ctx->fd, &ctx->segments[i], sizeof(struct segment_entry)) != 
            sizeof(struct segment_entry))
            return 0;
    }
    return 1;
}

static int write_memory_contents(struct coredump_ctx *ctx) {
    for (int i = 0; i < ctx->segment_count; i++) {
        if (!write_segment(ctx, &ctx->segments[i]))
            return 0;
    }
    return 1;
}

int generate_coredump(int signum) {
    struct coredump_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    if (!get_memory_segments(&ctx)) return 0;
    if (!create_core_file(&ctx)) goto cleanup_segments;
    
    setup_header(&ctx, signum);
    if (!calculate_offsets(&ctx)) goto cleanup;
    if (!write_header(&ctx)) goto cleanup;
    if (!write_segment_table(&ctx)) goto cleanup;
    if (!write_memory_contents(&ctx)) goto cleanup;
    
    close(ctx.fd);
    free(ctx.segments);
    return 1;
    
cleanup:
    close(ctx.fd);
    unlink(ctx.segments ? "core" : "");
cleanup_segments:
    free(ctx.segments);
    return 0;
}

void coredump_signal_handler(int signum) {
    generate_coredump(signum);
    _exit(128 + signum);
}

void install_coredump_handler() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = coredump_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;
    
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
}