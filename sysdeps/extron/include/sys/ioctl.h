#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#include <abi-bits/ioctls.h>

#ifdef __cplusplus
extern "C" {
#endif

int ioctl(int fd, unsigned long request, ...);

#ifdef __cplusplus
}
#endif

#endif // _SYS_IOCTL_H