/* baSic_ - kernel/elf.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 *
 * ELF32 static executable loader
 * reads PT_LOAD segments, copies them to their virtual addresses,
 * zero-fills the BSS region, returns the entry point
 */

#include "elf.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

u32 elf_load(const u8 *data, u32 size)
{
    if (size < sizeof(elf32_ehdr_t)) {
        kprintf("[ERR] elf: file too small\n");
        return 0;
    }

    const elf32_ehdr_t *ehdr = (const elf32_ehdr_t *)data;

    /* verify magic */
    if (*(u32 *)ehdr->e_ident != ELF_MAGIC) {
        kprintf("[ERR] elf: bad magic\n");
        return 0;
    }

    if (ehdr->e_type != ET_EXEC || ehdr->e_machine != EM_386) {
        kprintf("[ERR] elf: not an x86 executable\n");
        return 0;
    }

    /* walk program headers and load PT_LOAD segments */
    for (u16 i = 0; i < ehdr->e_phnum; i++) {
        const elf32_phdr_t *phdr =
            (const elf32_phdr_t *)(data + ehdr->e_phoff + i * ehdr->e_phentsize);

        if (phdr->p_type != PT_LOAD)
            continue;

        if (phdr->p_offset + phdr->p_filesz > size) {
            kprintf("[ERR] elf: segment out of bounds\n");
            return 0;
        }

        /* copy file bytes to virtual address */
        memcpy((void *)phdr->p_vaddr, data + phdr->p_offset, phdr->p_filesz);

        /* zero-fill the BSS region (memsz > filesz) */
        if (phdr->p_memsz > phdr->p_filesz)
            memset((void *)(phdr->p_vaddr + phdr->p_filesz),
                   0,
                   phdr->p_memsz - phdr->p_filesz);
    }

    kprintf("[OK] elf: loaded, entry %x\n", ehdr->e_entry);
    return ehdr->e_entry;
}