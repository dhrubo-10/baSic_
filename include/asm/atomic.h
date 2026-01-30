#ifndef _ASM_ATOMIC_H
#define _ASM_ATOMIC_H

#include <kernel_b/types.h>

typedef struct {
    volatile int counter;
} atomic_t;

#define ATOMIC_INIT(i)  { (i) }

static inline int atomic_read(const atomic_t *v)
{
    return v->counter;
}

static inline void atomic_set(atomic_t *v, int i)
{
    v->counter = i;
}

static inline void atomic_add(int i, atomic_t *v)
{
    __asm__ __volatile__(
        "lock addl %1, %0"
        : "+m" (v->counter)
        : "ir" (i)
    );
}

static inline void atomic_sub(int i, atomic_t *v)
{
    __asm__ __volatile__(
        "lock subl %1, %0"
        : "+m" (v->counter)
        : "ir" (i)
    );
}

static inline void atomic_inc(atomic_t *v)
{
    __asm__ __volatile__(
        "lock incl %0"
        : "+m" (v->counter)
    );
}

static inline void atomic_dec(atomic_t *v)
{
    __asm__ __volatile__(
        "lock decl %0"
        : "+m" (v->counter)
    );
}

static inline int atomic_inc_return(atomic_t *v)
{
    int result;
    __asm__ __volatile__(
        "lock incl %0; movl %0, %1"
        : "+m" (v->counter), "=r" (result)
    );
    return result;
}

static inline int atomic_dec_return(atomic_t *v)
{
    int result;
    __asm__ __volatile__(
        "lock decl %0; movl %0, %1"
        : "+m" (v->counter), "=r" (result)
    );
    return result;
}

static inline int atomic_add_return(int i, atomic_t *v)
{
    int result;
    __asm__ __volatile__(
        "lock addl %2, %0; movl %0, %1"
        : "+m" (v->counter), "=r" (result)
        : "ir" (i)
    );
    return result;
}

static inline int atomic_sub_return(int i, atomic_t *v)
{
    int result;
    __asm__ __volatile__(
        "lock subl %2, %0; movl %0, %1"
        : "+m" (v->counter), "=r" (result)
        : "ir" (i)
    );
    return result;
}

static inline int atomic_inc_and_test(atomic_t *v)
{
    unsigned char c;
    __asm__ __volatile__(
        "lock incl %0; sete %1"
        : "+m" (v->counter), "=qm" (c)
    );
    return c != 0;
}

static inline int atomic_dec_and_test(atomic_t *v)
{
    unsigned char c;
    __asm__ __volatile__(
        "lock decl %0; sete %1"
        : "+m" (v->counter), "=qm" (c)
    );
    return c != 0;
}

static inline int atomic_add_negative(int i, atomic_t *v)
{
    unsigned char c;
    __asm__ __volatile__(
        "lock addl %2, %0; sets %1"
        : "+m" (v->counter), "=qm" (c)
        : "ir" (i)
    );
    return c;
}

#define atomic_cmpxchg(v, old, new) ({ \
    __typeof__(*(v)) __ret; \
    __typeof__(*(v)) __old = (old); \
    __typeof__(*(v)) __new = (new); \
    __asm__ __volatile__( \
        "lock cmpxchgl %2, %1" \
        : "=a" (__ret), "+m" (*(v)) \
        : "r" (__new), "0" (__old) \
        : "memory" \
    ); \
    __ret; \
})

#define atomic_xchg(v, new) ({ \
    __typeof__(*(v)) __ret; \
    __typeof__(*(v)) __new = (new); \
    __asm__ __volatile__( \
        "xchgl %0, %1" \
        : "=r" (__ret), "+m" (*(v)) \
        : "0" (__new) \
        : "memory" \
    ); \
    __ret; \
})

#define atomic_fetch_add(i, v) ({ \
    __typeof__(*(v)) __ret; \
    __asm__ __volatile__( \
        "lock xaddl %0, %1" \
        : "=r" (__ret), "+m" (*(v)) \
        : "0" (i) \
        : "memory" \
    ); \
    __ret; \
})

#define atomic_fetch_sub(i, v) atomic_fetch_add(-(i), v)

#define smp_mb() __asm__ __volatile__("mfence" ::: "memory")
#define smp_rmb() __asm__ __volatile__("lfence" ::: "memory")
#define smp_wmb() __asm__ __volatile__("sfence" ::: "memory")

#define barrier() __asm__ __volatile__("" ::: "memory")

#endif 