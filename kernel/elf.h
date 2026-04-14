/* baSic_ - kernel/elf.h
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * ELF32 loader: parses and loads static ELF executables into memory
 */

#ifndef ELF_H
#define ELF_H

#include "../include/types.h"

/* ELF magic */
#define ELF_MAGIC   0x464C457F  /* "\x7FELF" as little-endian u32 */

/* e_type */
#define ET_EXEC     2

/* e_machine */
#define EM_386      3

/* program header p_type */
#define PT_LOAD     1

/* ELF32 file header */
typedef struct {
    u8  e_ident[16];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u32 e_entry;        /* virtual entry point */
    u32 e_phoff;        /* program header table offset */
    u32 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;        /* number of program headers */
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
} PACKED elf32_ehdr_t;

/* ELF32 program header */
typedef struct {
    u32 p_type;
    u32 p_offset;       /* offset in file */
    u32 p_vaddr;        /* virtual address to load at */
    u32 p_paddr;
    u32 p_filesz;       /* bytes in file */
    u32 p_memsz;        /* bytes in memory (>= filesz, zero-pad rest) */
    u32 p_flags;
    u32 p_align;
} PACKED elf32_phdr_t;

/* returns entry point address on success, 0 on failure */
u32 elf_load(const u8 *data, u32 size);

#endif