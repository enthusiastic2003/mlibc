#ifndef _EXTRON_SYS_SYSCALL_H
#define _EXTRON_SYS_SYSCALL_H

/* Extron's system call numbers are its own (see the kernel's syscall.h), not
 * Linux's, so no SYS_<name> macros are defined here: portable code that tests
 * `#ifdef SYS_gettid` and the like then falls back to the libc interfaces. */

#ifdef __cplusplus
extern "C" {
#endif

long syscall(long __number, ...);

#ifdef __cplusplus
}
#endif

#endif
