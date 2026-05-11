#ifndef _EXTRON_ABI_BITS_TYPES_H
#define _EXTRON_ABI_BITS_TYPES_H

// x86_64-elf-gcc defines all int_fast{8,16,32} as int (4 bytes)
#define __mlibc_int_fast8   int
#define __mlibc_int_fast16  int
#define __mlibc_int_fast32  int
#define __mlibc_uint_fast8  unsigned int
#define __mlibc_uint_fast16 unsigned int
#define __mlibc_uint_fast32 unsigned int

#endif // _EXTRON_ABI_BITS_TYPES_H