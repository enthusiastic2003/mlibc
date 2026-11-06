#include <errno.h>

int __thread __mlibc_errno;

char *program_invocation_name = nullptr;
char *program_invocation_short_name = nullptr;
// Separate variables, not aliases: an executable may copy-relocate one name (e.g.
// __progname), and then only assignments to that very name reach its copy.
char *__progname = nullptr;
char *__progname_full = nullptr;

int *__errno_location() {
	return &__mlibc_errno;
}
