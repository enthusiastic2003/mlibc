#ifndef _ERRNO_H
#define _ERRNO_H

#include <abi-bits/errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __MLIBC_ABI_ONLY

int *__errno_location(void);

/* Some programs define their own errno as an "extern int" if it is not a macro. */
#if defined(__TINYC__)
/* tcc's linker cannot import a thread-local variable from a shared library, so tcc-built
 * programs reach errno through the (exported) function that returns its address. */
#define errno (*__errno_location())
#else
#define errno __mlibc_errno
extern __thread int __mlibc_errno;
#endif

/* Linux extensions. */

extern char *program_invocation_name;
extern char *program_invocation_short_name;
extern char *__progname;
extern char *__progname_full;

#endif /* !__MLIBC_ABI_ONLY */

#ifdef __cplusplus
}
#endif

#endif /* _ERRNO_H */
