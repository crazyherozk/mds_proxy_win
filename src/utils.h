#ifndef __external_utils_h__
#define __external_utils_h__

/*
 * 平台检测
 */
#ifndef __BEGIN_DECLS
# if defined(__cplusplus)
#  define __BEGIN_DECLS    extern "C" {
#  define __END_DECLS    }
# else
#  define __BEGIN_DECLS
#  define __END_DECLS
# endif
#endif

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>

#include <time.h>
#include <sys/timeb.h>
#include <winsock2.h>
#include <winerror.h>
#include <ws2tcpip.h>
#ifdef EVENT__HAVE_AFUNIX_H
#include <afunix.h>
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <io.h>
#include <tchar.h>
#include <process.h>
#include <synchapi.h>

#ifndef _WIN32_WINNT
/* For structs needed by GetAdaptersAddresses */
# define _WIN32_WINNT 0x0600
#endif
#include <iphlpapi.h>
#include <netioapi.h>

__BEGIN_DECLS

#ifndef likely
# define likely(x)     (!!(x))
# define unlikely(x)   (!!(x))
#endif

#if !defined(__cplusplus) && !defined(inline)
# define inline __inline
#endif

#ifndef __always_inline
# define __always_inline inline
#endif

#if __LP64__ 
# define BITS_PER_LONG 64
# define BITS_PER_LONG_SHIFT 6
#else
# define BITS_PER_LONG 32
# define BITS_PER_LONG_SHIFT 5
#endif

#define U32_MAX		((uint32_t)~0U)

typedef long long ssize_t;
struct _iovec {
	void *   iov_base;
	size_t   iov_len;
};

#define __thread __declspec(thread)
#define cpu_relax() YieldProcessor()
#define static_mb() MemoryBarrier()

#define smp_mb() __faststorefence()
#define smp_rmb smp_mb
#define smp_wmb smp_mb

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

#ifndef offsetof
# ifdef __compiler_offsetof
#  define offsetof(TYPE, MEMBER)    __compiler_offsetof(TYPE, MEMBER)
# else
#  define offsetof(TYPE, MEMBER)    ((size_t)&((TYPE *)0)->MEMBER)
# endif
#endif

#ifndef container_of
# define container_of(ptr, type, member) \
		((type*)((uintptr_t)(ptr) - offsetof(type, member)))
#endif

static __always_inline void __read_once_size(const volatile void *p, void *res,
        size_t size)
{
    switch (size) {
        case 1: *(uint8_t*)res = *(volatile uint8_t*)p; break;
        case 2: *(uint16_t*)res = *(volatile uint16_t*)p; break;
        case 4: *(uint32_t*)res = *(volatile uint32_t*)p; break;
        case 8: *(uint64_t*)res = *(volatile uint64_t*)p; break;
        default: abort();
    }
    static_mb();
}

/*ensure that data of variable been write to memory*/
static __always_inline void __write_once_size(volatile void *p, void *res,
        size_t size)
{
    switch (size) {
        case 1: *(volatile uint8_t*)p = *(uint8_t *)res; break;
        case 2: *(volatile uint16_t *)p = *(uint16_t *)res; break;
        case 4: *(volatile uint32_t *)p = *(uint32_t *)res; break;
        case 8: *(volatile uint64_t *)p = *(uint64_t *)res; break;
        default: abort();
    }
    static_mb();
}

#define READ_ONCE(src, dst) __read_once_size(&(src), &(dst), sizeof(dst))
#define WRITE_ONCE(dst, src) __write_once_size(&(dst), &(src), sizeof(dst))

/*
 * 交换八字节
 * 0x1234567890abcdef => 0xefcdab9078563412
 */
static inline unsigned long long SWAP_8BYTES(unsigned long long v)
{
    return (((v & 0xff00000000000000ULL) >> 56) | ((v & 0x00ff000000000000) >> 40) |
        ((v & 0x0000ff0000000000ULL) >> 24) | ((v & 0x000000ff00000000) >> 8) |
        ((v & 0x00000000ff000000ULL) << 8) | ((v & 0x0000000000ff0000) << 24) |
        ((v & 0x000000000000ff00ULL) << 40) | ((v & 0x00000000000000ff) << 56));
}

/*64位 主机与网络字节序 转换
static inline long long _htonll(long long v)
{
# ifdef __BIG_ENDIAN
     return v;
# else
     return SWAP_8BYTES(v);
# endif
}

static inline long long _ntohll(long long v)
{
# ifdef __BIG_ENDIAN
     return v;
# else
     return SWAP_8BYTES(v);
# endif
}*/

const char *strerror_local(int code);
uint32_t byteCrc32(const void *, size_t, uint32_t);
uint64_t get_time(void);
int sched_yield(void);

ssize_t get_filesize(const char *);
ssize_t _pread(int fd, void *buf, size_t count, uint64_t offset);

__END_DECLS

#endif
