#ifndef _ASM_SPINLOCK_H
#define _ASM_SPINLOCK_H

#include <kernelb/types.h>

typedef struct {
    volatile unsigned int lock;
} spinlock_t;

#define SPIN_LOCK_UNLOCKED (spinlock_t){0}
#define spin_lock_init(lock) ((lock)->lock = 0)

static inline void spin_lock(spinlock_t *lock)
{
    __asm__ __volatile__(
        "1:\n\t"
        "lock btsl $0, %0\n\t"
        "jc 2f\n\t"
        ".subsection 1\n\t"
        ".type spin_lock_wait, @function\n"
        "spin_lock_wait:\n\t"
        "pause\n\t"
        "testl $1, %0\n\t"
        "jnz spin_lock_wait\n\t"
        "jmp 1b\n\t"
        ".previous\n"
        "2:"
        : "+m" (lock->lock)
        :
        : "memory"
    );
}

static inline void spin_unlock(spinlock_t *lock)
{
    __asm__ __volatile__(
        "movl $0, %0"
        : "+m" (lock->lock)
        :
        : "memory"
    );
}

static inline int spin_trylock(spinlock_t *lock)
{
    unsigned int old;
    
    __asm__ __volatile__(
        "movl $1, %%eax\n\t"
        "xchgl %%eax, %0"
        : "=m" (lock->lock), "=a" (old)
        :
        : "memory"
    );
    
    return old == 0;
}

static inline void spin_lock_irq(spinlock_t *lock)
{
    __asm__ __volatile__(
        "cli\n\t"
    );
    spin_lock(lock);
}

static inline void spin_unlock_irq(spinlock_t *lock)
{
    spin_unlock(lock);
    __asm__ __volatile__(
        "sti\n\t"
    );
}

static inline void spin_lock_irqsave(spinlock_t *lock, unsigned long *flags)
{
    __asm__ __volatile__(
        "pushf\n\t"
        "popl %0\n\t"
        "cli\n\t"
        : "=r" (*flags)
        :
        : "memory"
    );
    spin_lock(lock);
}

static inline void spin_unlock_irqrestore(spinlock_t *lock, unsigned long *flags)
{
    spin_unlock(lock);
    __asm__ __volatile__(
        "pushl %0\n\t"
        "popf\n\t"
        :
        : "r" (*flags)
        : "memory"
    );
}

static inline int spin_is_locked(spinlock_t *lock)
{
    return lock->lock != 0;
}

static inline void spin_lock_bh(spinlock_t *lock)
{
    __asm__ __volatile__(
        "cli\n\t"
    );
    spin_lock(lock);
}

static inline void spin_unlock_bh(spinlock_t *lock)
{
    spin_unlock(lock);
    __asm__ __volatile__(
        "sti\n\t"
    );
}

#define spin_lock_irqsave_nested(lock, flags, subclass) spin_lock_irqsave(lock, flags)

typedef struct {
    volatile unsigned int lock;
} rwlock_t;

#define RW_LOCK_UNLOCKED (rwlock_t){0}
#define rwlock_init(lock) ((lock)->lock = 0)

static inline void read_lock(rwlock_t *lock)
{
    __asm__ __volatile__(
        "1:\n\t"
        "lock incl %0\n\t"
        "jns 2f\n\t"
        "lock decl %0\n\t"
        "pause\n\t"
        "jmp 1b\n"
        "2:"
        : "+m" (lock->lock)
        :
        : "memory"
    );
}

static inline void read_unlock(rwlock_t *lock)
{
    __asm__ __volatile__(
        "lock decl %0"
        : "+m" (lock->lock)
        :
        : "memory"
    );
}

static inline void write_lock(rwlock_t *lock)
{
    __asm__ __volatile__(
        "1:\n\t"
        "lock btsl $31, %0\n\t"
        "jnc 2f\n\t"
        "pause\n\t"
        "jmp 1b\n"
        "2:"
        : "+m" (lock->lock)
        :
        : "memory"
    );
}

static inline void write_unlock(rwlock_t *lock)
{
    __asm__ __volatile__(
        "lock btrl $31, %0"
        : "+m" (lock->lock)
        :
        : "memory"
    );
}

static inline int write_trylock(rwlock_t *lock)
{
    unsigned int old;
    
    __asm__ __volatile__(
        "movl $0x80000000, %%eax\n\t"
        "lock xchgl %%eax, %0"
        : "=m" (lock->lock), "=a" (old)
        :
        : "memory"
    );
    
    return old == 0;
}

#endif 