#include <semaphore.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <bits/ensure.h>
#include <mlibc/debug.hpp>
#include <mlibc/ansi-sysdeps.hpp>
#include <mlibc/posix-sysdeps.hpp>

static constexpr unsigned int semaphoreHasWaiters = static_cast<uint32_t>(1 << 31);
static constexpr unsigned int semaphoreCountMask = static_cast<uint32_t>(1 << 31) - 1;

int sem_init(sem_t *sem, int, unsigned int initial_count) {
	// The count is a futex word, so a semaphore in shared memory works across
	// processes as long as the kernel keys futexes on shared pages.
	if (initial_count > SEM_VALUE_MAX) {
		errno = EINVAL;
		return -1;
	}

	sem->__mlibc_count = initial_count;

	return 0;
}

int sem_destroy(sem_t *) {
	return 0;
}

// Wait for the semaphore. `abstime` (on `clock`) bounds the wait when non-null.
static int sem_wait_until(sem_t *sem, const struct timespec *abstime, clockid_t clock) {
	unsigned int state = 0;

	while (1) {
		if (!(state & semaphoreCountMask)) {
			if (__atomic_compare_exchange_n(&sem->__mlibc_count, &state, semaphoreHasWaiters,
						false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)) {
				struct timespec rel;
				const struct timespec *relp = nullptr;
				if (abstime) {
					struct timespec now;
					if (clock_gettime(clock, &now))
						return -1;
					rel.tv_sec = abstime->tv_sec - now.tv_sec;
					rel.tv_nsec = abstime->tv_nsec - now.tv_nsec;
					if (rel.tv_nsec < 0) {
						rel.tv_sec--;
						rel.tv_nsec += 1000000000L;
					}
					if (rel.tv_sec < 0) {
						errno = ETIMEDOUT;
						return -1;
					}
					relp = &rel;
				}
				int e = mlibc::sys_futex_wait((int *)&sem->__mlibc_count, state, relp);
				if (e == 0 || e == EAGAIN || e == ETIMEDOUT) {
					continue;           // re-check the count; the deadline is re-tested above
				} else if (e == EINTR) {
					errno = EINTR;
					return -1;
				} else {
					mlibc::panicLogger() << "sys_futex_wait() failed with error code " << e << frg::endlog;
				}
			}
		} else {
			unsigned int desired = (state - 1);
			if (__atomic_compare_exchange_n(&sem->__mlibc_count, &state, desired, false,
						__ATOMIC_RELAXED, __ATOMIC_RELAXED))
				return 0;
		}
	}
}

int sem_wait(sem_t *sem) {
	return sem_wait_until(sem, nullptr, CLOCK_REALTIME);
}

int sem_timedwait(sem_t *sem, const struct timespec *abstime) {
	return sem_wait_until(sem, abstime, CLOCK_REALTIME);
}

int sem_clockwait(sem_t *sem, clockid_t clock, const struct timespec *abstime) {
	return sem_wait_until(sem, abstime, clock);
}

int sem_post(sem_t *sem) {
	auto old_count = __atomic_load_n(&sem->__mlibc_count, __ATOMIC_RELAXED) & semaphoreCountMask;

	if (old_count + 1 > SEM_VALUE_MAX) {
		errno = EOVERFLOW;
		return -1;
	}

	auto state = __atomic_exchange_n(&sem->__mlibc_count, old_count + 1, __ATOMIC_RELEASE);

	if (state & semaphoreHasWaiters)
		if (int e = mlibc::sys_futex_wake((int *)&sem->__mlibc_count); e)
			__ensure(!"sys_futex_wake() failed");

	return 0;
}

// Named semaphores live in a small file under /tmp, mapped MAP_SHARED, so every
// process that opens the name shares one futex word.
static bool sem_path(const char *name, char *buf, size_t size) {
	while (*name == '/')
		name++;
	size_t n = strlen(name);
	if (!n || strchr(name, '/') || n + 12 > size) {
		errno = n ? ENAMETOOLONG : EINVAL;
		return false;
	}
	memcpy(buf, "/tmp/.sem.", 10);
	memcpy(buf + 10, name, n + 1);
	return true;
}

sem_t *sem_open(const char *name, int oflag, ...) {
	mode_t mode = 0;
	unsigned int value = 0;
	if (oflag & O_CREAT) {
		va_list ap;
		va_start(ap, oflag);
		mode = va_arg(ap, mode_t);
		value = va_arg(ap, unsigned int);
		va_end(ap);
		if (value > SEM_VALUE_MAX) {
			errno = EINVAL;
			return SEM_FAILED;
		}
	}
	char path[NAME_MAX + 16];
	if (!sem_path(name, path, sizeof(path)))
		return SEM_FAILED;
	int fd = open(path, (oflag & (O_CREAT | O_EXCL)) | O_RDWR | O_CLOEXEC, mode);
	if (fd < 0)
		return SEM_FAILED;
	// The creator initialises the semaphore; the lock keeps openers from using it
	// before that is done.
	flock(fd, LOCK_EX);
	struct stat st;
	bool created = fstat(fd, &st) == 0 && st.st_size < 64;
	if (created && ftruncate(fd, 64)) {
		int e = errno;
		close(fd);
		errno = e;
		return SEM_FAILED;
	}
	void *map = mmap(nullptr, 64, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		int e = errno;
		close(fd);
		errno = e;
		return SEM_FAILED;
	}
	if (created)
		sem_init(static_cast<sem_t *>(map), 1, value);
	flock(fd, LOCK_UN);
	close(fd);
	return static_cast<sem_t *>(map);
}

int sem_close(sem_t *sem) {
	return munmap(sem, 64);
}

int sem_getvalue(sem_t *sem, int *out) {
	*out = static_cast<int>(__atomic_load_n(&sem->__mlibc_count, __ATOMIC_ACQUIRE) & semaphoreCountMask);
	return 0;
}

int sem_unlink(const char *name) {
	char path[NAME_MAX + 16];
	if (!sem_path(name, path, sizeof(path)))
		return -1;
	return unlink(path);
}

int sem_trywait(sem_t *sem) {
	while (true) {
		auto state = __atomic_load_n(&sem->__mlibc_count, __ATOMIC_ACQUIRE);

		if ((state & semaphoreHasWaiters) || !state) {
			errno = EAGAIN;
			return -1;
		}

		auto desired = state - 1;
		if (__atomic_compare_exchange_n(&sem->__mlibc_count, &state, desired, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED)) {
			return 0;
		}
	}
}
