#ifndef _LINK_H
#define _LINK_H

#include <stdint.h>
#include <stddef.h>
#include <elf.h>

#define ElfW(type) Elf64_##type

struct dl_phdr_info {
    Elf64_Addr        dlpi_addr;
    const char       *dlpi_name;
    const Elf64_Phdr *dlpi_phdr;
    Elf64_Half        dlpi_phnum;
    unsigned long long dlpi_adds;
    unsigned long long dlpi_subs;
    size_t            dlpi_tls_modid;
    void             *dlpi_tls_data;
};

struct dl_find_object {
    unsigned long long dlfo_flags;
    void *dlfo_map_start;
    void *dlfo_map_end;
    void *dlfo_link_map;
    void *dlfo_eh_frame;
};

#ifdef __cplusplus
extern "C" {
#endif
int dl_iterate_phdr(int (*callback)(struct dl_phdr_info *, size_t, void *), void *data);
#ifdef __cplusplus
}
#endif

#endif // _LINK_H