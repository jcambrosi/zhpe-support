/*
 * Copyright (C) 2017-2019 Hewlett Packard Enterprise Development LP.
 * All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ZHPEQ_UTIL_H_
#define _ZHPEQ_UTIL_H_

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <uuid/uuid.h>

#include <x86intrin.h>

#include <zhpe_externc.h>

_EXTERN_C_BEG

/* Type checking macros */
#ifdef container_of
#undef container_of
#endif
#define container_of(ptr, type, member)                         \
({                                                              \
    typeof( ((type *)0)->member ) *_ptr = (ptr);                \
    (type *)((char *)_ptr - offsetof(type, member));            \
})

#ifndef max
#undef max
#endif
#define max(_a, _b)                                             \
({                                                              \
    __auto_type         __ret = (_a);                           \
    __auto_type         __b = (_b);                             \
    /* Force compilation error if different types. */           \
    typeof(&__ret)      __p MAYBE_UNUSED;                       \
    __p = &__b;                                                 \
                                                                \
    if (__b > __ret)                                            \
        __ret = __b;                                            \
    __ret;                                                      \
})

#ifndef min
#undef min
#endif
#define min(_a, _b)                                             \
({                                                              \
    __auto_type         __ret = (_a);                           \
    __auto_type         __b = (_b);                             \
    /* Force compilation error if different types. */           \
    typeof(&__ret)      __p MAYBE_UNUSED;                       \
    __p = &__b;                                                 \
                                                                \
    if (__b < __ret)                                            \
        __ret = __b;                                            \
    __ret;                                                      \
})


#define arithcmp(_a, _b)                                        \
({                                                              \
    __auto_type         __a = (_a);                             \
    __auto_type         __b = (_b);                             \
    /* Force compilation error if different types. */           \
    typeof(&__a)        __p = &__b MAYBE_UNUSED;                \
    __p = &__b;                                                 \
                                                                \
     ((__a) < (__b) ? -1 : ((__a) > (__b) ? 1 : 0));            \
})

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_x)  (sizeof(_x) / sizeof(_x[0]))
#endif

#define TO_PTR(_int)    (void *)(uintptr_t)(_int)

#define FREE_IF(_ptr,_free)                                     \
do {                                                            \
    if (_ptr) {                                                 \
        _free(_ptr);                                            \
        (_ptr) = NULL;                                          \
    }                                                           \
} while (0)

#define FD_CLOSE(_fd)                                           \
({                                                              \
    int                 __ret = 0;                              \
                                                                \
    if ((_fd) >= 0) {                                           \
        __ret = close(_fd);                                     \
        (_fd) = -1;                                             \
    }                                                           \
    __ret;                                                      \
})

typedef long long       llong;
typedef unsigned long long ullong;
typedef unsigned char   uchar;

extern const char       *zhpeu_appname;
// #define appname zhpeu_appname

/* Borrow AF_APPLETALK since it should never be seen. */
#define AF_ZHPE         AF_APPLETALK
#define ZHPE_ADDRSTRLEN         (37)
#define ZHPE_WILDCARD           (0)     /* Valid, but reserved by driver. */
#define ZHPE_SZQ_INVAL          (~(uint32_t)0)

#define ZHPE_GCID_BITS          (28)
#define ZHPE_GCID_MASK          ((1U << ZHPE_GCID_BITS) - 1)
#define ZHPE_CTXID_BITS         (24)
#define ZHPE_CTXID_MASK         ((1U << ZHPE_CTXID_BITS) - 1)

#define ZHPE_SZQ_FLAGS_MASK     (0xFFU << ZHPE_CTXID_BITS)
#define ZHPE_SZQ_FLAGS_FAM      (1U << ZHPE_CTXID_BITS)

struct sockaddr_zhpe {
    sa_family_t         sz_family;
    uuid_t              sz_uuid;
    uint32_t            sz_queue;       /* Network byte order */
};

uint32_t zhpeu_uuid_to_gcid(const uuid_t uuid);
void zhpeu_install_gcid_in_uuid(uuid_t uuid, uint32_t gcid);
bool zhpeu_uuid_gcid_only(const uuid_t uuid);

union sockaddr_in46 {
    /* sa_family common to all, sin_port common to IPv4/6. */
    struct {
        sa_family_t     sa_family;
        in_port_t       sin_port;
    };
    struct sockaddr_in  addr4;
    struct sockaddr_in6 addr6;
    struct sockaddr_zhpe zhpe;
};

static_assert(sizeof(union sockaddr_in46) <= sizeof(struct sockaddr_in6),
              "sockaddr_in46 len");
static_assert(INET6_ADDRSTRLEN >= ZHPE_ADDRSTRLEN, "ZHPE_ADDRSTRLEN");

#ifdef __GNUC__

#ifdef _BARRIER_DEFINED
#warning _BARRIER_DEFINED already defined
#undef _BARRIER_DEFINED
#endif

#if defined(__x86_32__) || defined( __x86_64__)

#define _BARRIER_DEFINED

/*
 * But atomic_thread_fence() didn't generate the fences I wanted when I
 * tested it.
 */

static inline void smp_mb(void)
{
    _mm_mfence();
}

static inline void smp_rmb(void)
{
    _mm_lfence();
}

static inline void smp_wmb(void)
{
    _mm_sfence();
}

static inline void io_rmb(void)
{
    _mm_lfence();
}

static inline void io_wmb(void)
{
    _mm_sfence();
}

static inline void io_mb(void)
{
    _mm_mfence();
}

#define L1_CACHE_BYTES  (64UL)

static inline void nop(void)
{
    asm volatile("nop");
}

static inline int fls64(uint64_t v)
{
    int                 ret = -1;

    asm("bsrq %1,%q0" : "+r" (ret) : "r" (v));

    return ret;
}

#endif

#ifndef _BARRIER_DEFINED
#error No barrier support for this architecture
#endif

#undef _BARRIED_DEFINED

#define barrier()       __compiler_barrier()
#define INT32_ALIGNED   __attribute__ ((aligned (__alignof__(int32_t))));
#define INT64_ALIGNED   __attribute__ ((aligned (__alignof__(int64_t))));
#define INT128_ALIGNED  __attribute__ ((aligned (__alignof__(__int128_t))));
#define CACHE_ALIGNED   __attribute__ ((aligned (L1_CACHE_BYTES)))

#define MAYBE_UNUSED    __attribute__((unused))
#define NO_RETURN       __attribute__ ((__noreturn__))
#define PRINTF_ARGS(_a, _b) __attribute__ ((format (printf, _a, _b)))

#ifndef likely
#define likely(_x)      __builtin_expect(!!(_x), 1)
#define unlikely(_x)    __builtin_expect(!!(_x), 0)
#endif

#endif /* __GNUC__ */

void zhpeq_util_init(char *argv0, int default_log_level, bool use_syslog);
void zhpeu_print_dbg(const char *fmt, ...) PRINTF_ARGS(1, 2);
void zhpeu_print_info(const char *fmt, ...) PRINTF_ARGS(1, 2);
void zhpeu_print_err(const char *fmt, ...) PRINTF_ARGS(1, 2);
void zhpeu_print_usage(bool use_stdout, const char *fmt, ...) PRINTF_ARGS(2, 3);
void zhpeu_print_func_err(const char *callf, uint line, const char *errf,
                          const char *arg, int err);
void zhpeu_print_func_errn(const char *callf, uint line, const char *errf,
                           llong arg, bool arg_hex, int err);
void zhpeu_print_range_err(const char *callf, uint line, const char *name,
                           int64_t val, int64_t min, int64_t max);
void zhpeu_print_urange_err(const char *callf, uint line, const char *name,
                            uint64_t val, uint64_t min, uint64_t max);

void zhpeu_fatal(const char *callf, uint line, const char *errf, int ret);
void zhpeu_err(const char *callf, uint line, const char *errf, int ret);
void zhpeu_dbg(const char *callf, uint line, const char *errf, int ret);

#define zhpeu_syscall(_err_handler, _func, ...)                 \
({                                                              \
    long                 __ret = _func(__VA_ARGS__);            \
                                                                \
    if (unlikely(__ret == -1)) {                                \
        __ret = -errno;                                         \
        _err_handler(__func__, __LINE__, #_func, __ret);        \
    }                                                           \
    __ret;                                                      \
})

#define zhpeu_posixcall(_err_handler, _func, ...)               \
({                                                              \
    int                 __ret = -_func(__VA_ARGS__);            \
                                                                \
    if (unlikely(__ret))                                        \
        _err_handler(__func__, __LINE__, #_func, __ret);        \
    __ret;                                                      \
})

#define zhpeu_posixcall_errorok(_err_handler, _func, _err, ...) \
({                                                              \
    int                 __ret = -_func(__VA_ARGS__);            \
    int                 __err = (_err);                         \
                                                                \
    if (unlikely(__ret)&& __ret != __err)                       \
        _err_handler(__func__, __LINE__, #_func, __ret);        \
    __ret;                                                      \
})

#define zhpeu_call_neg(_err_handler, _func, ...)                \
({                                                              \
    int                 __ret = _func(__VA_ARGS__);             \
                                                                \
    if (unlikely(__ret < 0))                                    \
        _err_handler(__func__, __LINE__, #_func, __ret);        \
    __ret;                                                      \
})

#define zhpeu_call_null(_err_handler, _func, _rtype, ...)       \
({                                                              \
    _rtype              __ret = _func(__VA_ARGS__);             \
    int                 __saved_errno;                          \
                                                                \
    if (unlikely((void *)__ret == NULL)) {                      \
        __saved_errno = errno;                                  \
        _err_handler(__func__, __LINE__, #_func, -errno);       \
        errno = __saved_errno;                                  \
    }                                                           \
    __ret;                                                      \
})

static inline sa_family_t zhpeu_sockaddr_family(const void *addr)
{
    const union sockaddr_in46 *sa = addr;

    return sa->sa_family;
}

uint32_t zhpeu_sockaddr_porth(const void *addr);
size_t zhpeu_sockaddr_len(const void *addr);
bool zhpeu_sockaddr_valid(const void *addr, size_t addr_len, bool check_len);
void zhpeu_sockaddr_cpy(union sockaddr_in46 *dst, const void *src);
int zhpeu_sockaddr_portcmp(const void *addr1, const void *addr2);
int zhpeu_sockaddr_cmp(const void *addr1, const void *addr2);
bool zhpeu_sockaddr_inet(const void *addr);
bool zhpeu_sockaddr_wildcard(const void *addr);
bool zhpeu_sockaddr_loopback(const void *addr, bool loopany);
void zhpeu_sockaddr_6to4(void *addr);

const char *zhpeu_sockaddr_ntop(const void *addr, char *buf, size_t len);
char *zhpeu_sockaddr_str(const void *addr);

#define zhpeu_expected_saw(_lbl, _expected, _saw)               \
({                                                              \
    bool                __ret;                                  \
    const char          *__lbl = (_lbl);                        \
    __auto_type         __e = (_expected);                      \
    __auto_type         __s = (_saw);                           \
    /* Force compilation error if different types. */           \
    typeof(&__e)        __p MAYBE_UNUSED;                       \
    __p = &__s;                                                 \
                                                                \
    __ret = (__e == __s);                                       \
    if (!__ret) {                                               \
        zhpeu_print_err("%s,%u:%s expected 0x%llx, "            \
                        " saw 0x%llx\n", __func__, __LINE__,    \
                        __lbl, (ullong)__e, (ullong)__s);       \
    }                                                           \
    __ret;                                                      \
})

/* Just a call to free for things that need a function pointer. */
void zhpeu_free_ptr(void *ptr);

/* Trying to rely on stdatomic.h with less verbosity.
 * I'm not at all convinced they do the right thing with fences, in general,
 * but on x86 atomic adds and cmpxchg are full barriers. So the only relaxed
 * thing I use are loads/stores.
 */

#define atm_load(_p)                                            \
 atomic_load_explicit(_p, memory_order_acquire)
#define atm_load_rlx(_p)                                        \
    atomic_load_explicit(_p, memory_order_relaxed)

#define atm_store(_p, _v)                                       \
    atomic_store_explicit(_p, _v, memory_order_release)
#define atm_store_rlx(_p, _v)                                   \
    atomic_store_explicit(_p, _v, memory_order_relaxed)

#define atm_add(_p, _v)                                         \
    atomic_fetch_add_explicit(_p, _v, memory_order_acq_rel)

#define atm_and(_p, _v)                                         \
    atomic_fetch_and_explicit(_p, _v, memory_order_acq_rel)

#define atm_or(_p, _v)                                          \
    atomic_fetch_or_explicit(_p, _v, memory_order_acq_rel)

#define atm_sub(_p, _v)                                         \
    atomic_fetch_sub_explicit(_p, _v, memory_order_acq_rel)

#define atm_xchg(_p, _v)                                        \
    atomic_exchange_explicit(_p, _v, memory_order_acq_rel)

#define atm_xor(_p, _v)                                         \
    atomic_fetch_xor_explicit(_p, _v, memory_order_acq_rel)

#define atm_cmpxchg(_p, _oldp, _new)                            \
    atomic_compare_exchange_strong_explicit(                    \
        _p, _oldp, _new, memory_order_acq_rel, memory_order_acquire)

#define atm_inc(_p)     atm_add(_p, 1)
#define atm_dec(_p)     atm_sub(_p, 1)

/* Two simple atomic lists:
 * "lifo" for free lists and a "snatch" list with multiple-producers and
 * one consumer that snatches the entire list for processing at once: this
 * avoids most of the complexities with enqeue and dequeue around A-B-A, but
 * the tail must be handled carefully.
 */

struct zhpeu_atm_list_ptr {
    struct zhpeu_atm_list_next *ptr;
    uintptr_t            seq;
} INT128_ALIGNED;

struct zhpeu_atm_list_next {
    struct zhpeu_atm_list_next *next;
} INT64_ALIGNED;

struct zhpeu_atm_snatch_head {
    struct zhpeu_atm_list_next *head;
    struct zhpeu_atm_list_next *tail;
} INT128_ALIGNED;

#define ZHPEU_ATM_LIST_END      ((struct zhpeu_atm_list_next *)(intptr_t)-1)

static inline void zhpeu_atm_snatch_insert(struct zhpeu_atm_snatch_head *head,
                                           struct zhpeu_atm_list_next *new)
{
    struct zhpeu_atm_snatch_head oldh;
    struct zhpeu_atm_snatch_head newh;
    struct zhpeu_atm_list_next oldn;

    new->next = NULL;
    for (oldh = atm_load_rlx(head);;) {
        if (oldh.head) {
            newh.head = oldh.head;
            /* Try to link new into list. */
            oldn.next = NULL;
            if (!atm_cmpxchg(&oldh.tail->next, &oldn, new)) {
                /* Failed: advance the tail ourselves and retry. */
                newh.tail = oldn.next;
                if (atm_cmpxchg(head, &oldh, newh))
                    oldh = newh;
                continue;
            }
            /* Try to update the head; succeed or fail, we're done.
             * If we fail, it is up to the other threads to deal with it.
             */
            newh.tail = new;
            atm_cmpxchg(head, &oldh, newh);
            break;
        }
        /* List was empty. */
        newh.head = new;
        newh.tail = new;
        if (atm_cmpxchg(head, &oldh, newh))
            break;
    }
}

static inline void zhpeu_atm_snatch_list(struct zhpeu_atm_snatch_head *head,
                                         struct zhpeu_atm_snatch_head *oldh)
{
    struct zhpeu_atm_snatch_head newh;
    struct zhpeu_atm_list_next oldn;

    for (*oldh = atm_load_rlx(head);;) {
        if (!oldh->head)
            return;
        newh.head = NULL;
        newh.tail = NULL;
        if (atm_cmpxchg(head, oldh, newh))
            break;
    }
    /*
     * Worst case: another thread has copied the head and went to sleep
     * before updating the next pointer and will wake up at some point far
     * in the future and do so. Or another thread could have successfully
     * updated next, but the tail update failed. We update the final next
     * pointer with ZHPEU_ATM_LIST_END to deal with some of this, but the
     * potential for a thread lurking demands a more structural
     * solution. The fifo list will also use ZHPEU_ATM_LIST_END, instead of
     * NULL and the assumption is that items will be bounced between
     * snatch lists and fifos as free lists; items will never be returned
     * to a general allocation pool unless some broader guarantee that
     * it is safe to do so.
     */
    for (;;) {
        oldn.next = NULL;
        if (atm_cmpxchg(&oldh->tail->next, &oldn, ZHPEU_ATM_LIST_END))
            break;
        oldh->tail = oldn.next;
    }
}

static inline void zhpeu_atm_fifo_init(struct zhpeu_atm_list_ptr *head)
{
    head->ptr = ZHPEU_ATM_LIST_END;
    head->seq = 0;
}

static inline void zhpeu_atm_fifo_push(struct zhpeu_atm_list_ptr *head,
                                       struct zhpeu_atm_list_next *new)
{
    struct zhpeu_atm_list_ptr oldh;
    struct zhpeu_atm_list_ptr newh;

    newh.ptr = new;
    for (oldh = atm_load_rlx(head);;) {
        new->next = oldh.ptr;
        newh.seq = oldh.seq + 1;
        if (atm_cmpxchg(head, &oldh, newh))
            break;
    }
}

static inline struct zhpeu_atm_list_next *
zhpeu_atm_fifo_pop(struct zhpeu_atm_list_ptr *head)
{
    struct zhpeu_atm_list_next *ret;
    struct zhpeu_atm_list_ptr oldh;
    struct zhpeu_atm_list_ptr newh;

    for (oldh = atm_load_rlx(head);;) {
        ret = oldh.ptr;
        if (ret == ZHPEU_ATM_LIST_END) {
            ret = NULL;
            break;
        }
        newh.ptr = ret->next;
        newh.seq = oldh.seq + 1;
        if (atm_cmpxchg(head, &oldh, newh))
            break;
    }

    return ret;
}

struct zhpeu_init_time {
    uint64_t            (*get_cycles)(volatile uint32_t *cpup);
    uint64_t            freq;
    void                (*clflush_range)(const void *p, size_t len, bool fence);
    void                (*clwb_range)(const void *p, size_t len, bool fence);
    uint64_t            pagesz;
    uint64_t            l1sz;
};

extern struct zhpeu_init_time *zhpeu_init_time;

#define page_size       (zhpeu_init_time->pagesz)

#define NSEC_PER_SEC    (1000000000UL)
#define NSEC_PER_USEC   (1000000UL)

static inline double cycles_to_usec(uint64_t delta, uint64_t loops)
{
    return (((double)delta * NSEC_PER_USEC) /
            ((double)zhpeu_init_time->freq * loops));
}

static inline uint64_t get_cycles(volatile uint32_t *cpup)
{
    return zhpeu_init_time->get_cycles(cpup);
}

static inline uint64_t get_tsc_freq(void)
{
    return zhpeu_init_time->freq;
}

static inline void clflush_range(const void *addr, size_t length, bool fence)
{
    zhpeu_init_time->clflush_range(addr, length, fence);
}

static inline void clwb_range(const void *addr, size_t length,  bool fence)
{
    zhpeu_init_time->clwb_range(addr, length, fence);
}

#define clock_gettime(...)                                      \
    zhpeu_syscall(zhpeu_fatal, clock_gettime, __VA_ARGS__)

#define clock_gettime_monotonic(...)                            \
    clock_gettime(CLOCK_MONOTONIC, __VA_ARGS__)

static inline uint64_t ts_delta(struct timespec *ts_beg,
                                struct timespec *ts_end)
{
    return ((uint64_t)1000000000) * (ts_end->tv_sec - ts_beg->tv_sec) +
        (ts_end->tv_nsec - ts_beg->tv_nsec);
}

enum {
    PARSE_NUM           = 0,
    PARSE_KB            = 1,
    PARSE_KIB           = 2,
};

int zhpeu_parse_kb_uint64_t(const char *name, const char *sp, uint64_t *val,
                            int base, uint64_t min, uint64_t max, int flags);

enum {
    CHECK_EAGAIN_OK     = 1,
    CHECK_SHORT_IO_OK   = 2,
};

int zhpeu_check_func_io(const char *callf, uint line, const char *errf,
                        const char *arg, size_t req, ssize_t res,
                        int flags);
int zhpeu_check_func_ion(const char *callf, uint line, const char *errf,
                         long arg, bool arg_hex, size_t req, ssize_t res,
                         int flags);

int zhpeu_sock_getaddrinfo(const char *node, const char *service,
                           int family, int socktype, bool passive,
                           struct addrinfo **res);
int zhpeu_sock_getsockname(int fd, union sockaddr_in46 *sa);
int zhpeu_sock_getpeername(int fd, union sockaddr_in46 *da);
int zhpeu_sock_connect(const char *node, const char *service);
int zhpeu_sock_send_blob(int fd, const void *blob, size_t blob_len);
int zhpeu_sock_recv_fixed_blob(int fd, void *blob, size_t blob_len);
int zhpeu_sock_recv_var_blob(int fd, size_t extra_len,
                             void **blob, size_t *blob_len);
int zhpeu_sock_send_string(int fd, const char *s);
int zhpeu_sock_recv_string(int fd, char **s);

void zhpeu_random_seed(uint seed);
uint zhpeu_random_range(uint start, uint end);
uint *zhpeu_random_array(uint *array, uint entries);

int zhpeu_munmap(void *addr, size_t length);
int zhpeu_mmap(void **addr, size_t length, int prot, int flags,
               int fd, off_t offset);

char *zhpeu_get_cpuinfo_val(FILE *fp, char *buf, size_t buf_size,
                            uint field, const char *name, ...);

/* Calls where errors should *never* really happen. */

#define cond_init(...)                                          \
    zhpeu_posixcall(zhpeu_fatal, pthread_cond_init, __VA_ARGS__)
#define cond_destroy(...)                                       \
    zhpeu_posixcall(zhpeu_fatal, pthread_cond_destroy, __VA_ARGS__)
#define cond_signal(...)                                        \
    zhpeu_posixcall(zhpeu_fatal, pthread_cond_signal, __VA_ARGS__)
#define cond_broadcast(...)                                     \
    zhpeu_posixcall(zhpeu_fatal, pthread_cond_broadcast, __VA_ARGS__)
#define cond_wait(...)                                          \
    zhpeu_posixcall(zhpeu_fatal, pthread_cond_wait, __VA_ARGS__)
#define cond_timedwait(...)                                     \
    zhpeu_posixcall_errorok(                                    \
        zhpeu_fatal, pthread_cond_timedwait, -ETIMEDOUT, __VA_ARGS__)
#define mutexattr_settype(...)                                  \
    zhpeu_posixcall(zhpeu_fatal, pthread_mutexattr_settype, __VA_ARGS__)
#define mutexattr_init(...)                                     \
    zhpeu_posixcall(zhpeu_fatal, pthread_mutexattr_init, __VA_ARGS__)
#define mutexattr_destroy(...)                                  \
    zhpeu_posixcall(zhpeu_fatal, pthread_mutexattr_destroy, __VA_ARGS__)
#define mutex_init(...)                                         \
    zhpeu_posixcall(zhpeu_fatal, pthread_mutex_init, __VA_ARGS__)
#define mutex_destroy(...)                                      \
    zhpeu_posixcall(zhpeu_fatal, pthread_mutex_destroy, __VA_ARGS__)
#define mutex_lock(...)                                         \
    zhpeu_posixcall(zhpeu_fatal, pthread_mutex_lock, __VA_ARGS__)
#define mutex_trylock(...)                                      \
    zhpeu_posixcall_errorok(pthread_mutex_trylock, EBUSY, __VA_ARGS__)
#define mutex_unlock(...)                                       \
    zhpeu_posixcall(zhpeu_fatal, pthread_mutex_unlock, __VA_ARGS__)
#define spin_init(...)                                          \
    zhpeu_posixcall(zhpeu_fatal, pthread_spin_init, __VA_ARGS__)
#define spin_destroy(...)                                       \
    zhpeu_posixcall(zhpeu_fatal, pthread_spin_destroy, __VA_ARGS__)
#define spin_lock(...)                                          \
    zhpeu_posixcall(zhpeu_fatal, pthread_spin_lock, __VA_ARGS__)
#define spin_unlock(...)                                        \
    zhpeu_posixcall(zhpeu_fatal, pthread_spin_unlock, __VA_ARGS__)

/* Keep _GNU_SOURCE out of the headers. */

char *zhpeu_asprintf(const char *fmt, ...) PRINTF_ARGS(1, 2);
#define _zhpeu_asprintf(...)                                    \
    zhpeu_call_null(zhpeu_fatal, zhpeu_asprintf, __VA_ARGS__)

int zhpeu_yield(void);
#define _zhpeu_yield(...)                                       \
    zhpeu_posixcall(zhpeu_fatal, zhpeu_yield, __VA_ARGS__)

/* publib-like no-fail APIs without publib. */

static inline void *xmalloc(size_t size)
{
    void                *ret;

    if (likely(size))
        ret = malloc(size);
    else
        ret = NULL;

    return ret;
}

static inline void *xrealloc(void *ptr, size_t size)
{
    void                *ret;

    if (likely(ptr && size))
        ret = realloc(ptr, size);
    else
        ret = NULL;

    return ret;
}

static inline void *xcalloc(size_t nmemb, size_t size)
{
    void                *ret;

    /* Revisit:add check for overflow? */
    size *= nmemb;
    if (likely(size))
        ret = malloc(size);
    else
        ret = NULL;

    return ret;
}

void zhpeu_yield(void);

#define yield()         zhpeu_yield()

#define xposix_memalign(...)                                    \
    zhpeu_posixcall(zhpeu_fatal, posix_memalign, __VA_ARGS__)
#define xmalloc(...)                                            \
    zhpeu_call_null(zhpeu_fatal, xmalloc, void *, __VA_ARGS__)
#define xrealloc(...)                                           \
    zhpeu_call_null(zhpeu_fatal, xrealloc, void *, __VA_ARGS__)
#define xcalloc(...)                                            \
    zhpeu_call_null(zhpeu_fatal, xcalloc, void *, __VA_ARGS__)
#define xasprintf(...)                                          \
    zhpeu_syscall(zhpeu_fatal, asprintf, __VA_ARGS__)

static inline void *xmalloc_aligned(size_t alignment, size_t size)
{
    void                *ret;

    errno = posix_memalign(&ret, alignment, size);
    if (unlikely(errno))
        ret = NULL;

    return ret;
}

static inline void *xcalloc_aligned(size_t alignment, size_t nmemb, size_t size)
{
    void                *ret;

    /* Revisit:add check for overflow? */
    size *= nmemb;
    ret = xmalloc_aligned(alignment, size);
    if (likely(ret))
        memset(ret, 0, size);

    return ret;
}

#define xmalloc_aligned(...)                                    \
    zhpeu_call_null(zhpeu_fatal, xmalloc_aligned, void *, __VA_ARGS__)
#define xmalloc_cachealigned(_size)                             \
    xmalloc_aligned(L1_CACHE_BYTES, (_size))
#define xcalloc_aligned(...)                                    \
    zhpeu_call_null(zhpeu_fatal, xcalloc_aligned, void *, __VA_ARGS__)
#define xcalloc_cachealigned(_nmemb, _size)                     \
    xcalloc_aligned(L1_CACHE_BYTES, (_nmemb), (_size))

#define xstrdup(_s)                                             \
({                                                              \
    void                *__ret;                                 \
    const char          *__s = (_s);                            \
                                                                \
    if (likely(__s))                                            \
        __ret = zhpeu_call_null(zhpeu_fatal, strdup,            \
                                char *, s);                     \
    else                                                        \
        __ret = NULL;                                           \
                                                                \
    return __ret;                                               \
})

#define xmemdup(_mem, _bytes)                                   \
({                                                              \
    void                *__ret;                                 \
    const void          *__mem = (_mem);                        \
    size_t              __bytes = (_bytes);                     \
                                                                \
    if (likely(__mem && __bytes)) {                             \
        __ret = xmalloc(__bytes);                               \
        memcpy(__ret, __mem, __bytes);                          \
    }                                                           \
                                                                \
    return __ret;                                               \
})

/*
 * Wrappers for calls where errors may not necessaily be fatal.
 * Leading '_' allows callers a choice to not use the wrappers.
 */

#define _zhpeu_sockaddr_ntop(...)                               \
    zhpeu_call_null(zhpeu_sockaddr_ntop, char *, __VA_ARGS__)
#define _zhpeu_sockaddr_str(...)                                \
    zhpeu_call_null(zhpeu_dbg, zhpeu_sockaddr_str, char *, __VA_ARGS__)
#define _zhpeu_get_cpu_info_val(...)                            \
    zhpeu_call_null(zhpeu_dbg, zhpeu_get_cpuinfo_val, char *, __VA_ARGS__)
#define _zhpeu_parse_kb_uint64_t(...)                           \
    zhpeu_call_null(zhpeu_dbg, zhpeu_parse_kb_uint64_t, char *, __VA_ARGS__)
#define _zhpeu_sock_getaddrinfo(...)                            \
    zhpeu_call_neg(zhpeu_dbg, zhpeu_sock_getaddrinfo, __VA_ARGS__)
#define _zhpeu_sock_connect(...)                                \
    zhpeu_call_neg(zhpeu_dbg, zhpeu_sock_connect, __VA_ARGS__)
#define _zhpeu_sock_getsockname(...)                            \
    zhpeu_call_neg(zhpeu_dbg, zhpeu_sock_getsockname, __VA_ARGS__)
#define _zhpeu_sock_getpeername(...)                            \
    zhpeu_call_neg(zhpeu_dbg, zhpeu_sock_getpeername, __VA_ARGS__)
#define _zhpeu_sock_send_blob(...)                              \
    zhpeu_call_neg(zhpeu_dbg, zhpeu_sock_send_blob, __VA_ARGS__)
#define _zhpeu_sock_recv_fixed_blob(...)                        \
    zhpeu_call_neg(zhpeu_dbg, zhpeu_recv_fixed_blob, __VA_ARGS__)
#define _zhpeu_sock_recv_var_blob(...)                          \
    zhpeu_call_neg(zhpeu_dbg, zhpeu_recv_var_blob, __VA_ARGS__)
#define _zhpeu_sock_send_string(...)                            \
    zhpeu_call_neg(zhpeu_dbg, zhpeu_sock_send_string, __VA_ARGS__)
#define _zhpeu_sock_recv_string(...)                            \
    zhpeu_call_neg(zhpeu_dbg, zhpeu_sock_recv_string, __VA_ARGS__)
#define _zhpeu_munmap(...)                                      \
    zhpeu_call_neg(zhpeu_dbg, zhpeu_munmap, __VA_ARGS__)
#define _zhpeu_mmap(...)                                        \
    zhpeu_call_neg(zhpeu_dbg, zhpeu_mmap, __VA_ARGS__)
#define _zhpeu_get_cpuinfo_val(...)                             \
    zhpeu_call_null(zhpeu_dbg, zhpeu_get_cpuinfo_val, void *, __VA_ARGS__)

static inline uint64_t roundup64(uint64_t val, uint64_t round)
{
    return ((val + round - 1) / round * round);
}

static inline uint64_t roundup_pow_of_2(uint64_t val)
{
    if (!val || !(val & (val - 1)))
        return val;

    return ((uint64_t)1 << (fls64(val) + 1));
}

static inline uint64_t page_off(uint64_t addr)
{
    uint64_t            page_off_mask = (uint64_t)(page_size - 1);

    return (addr & page_off_mask);
}

static inline uint64_t page_down(uint64_t addr)
{
    uint64_t            page_mask = ~(uint64_t)(page_size - 1);

    return (addr & page_mask);
}

static inline uint64_t page_up(uint64_t addr)
{
    uint64_t            page_mask = ~(uint64_t)(page_size - 1);

    return (((addr + page_size - 1) & page_mask));
}

struct zhpeu_thr_wait {
    int32_t             state;
    pthread_mutex_t     mutex;
    pthread_cond_t      cond;
} CACHE_ALIGNED;

#define MS_PER_SEC      (1000UL)
#define US_PER_SEC      (1000000UL)
#define NS_PER_SEC      (1000000000UL)

enum {
    ZHPEU_THR_WAIT_IDLE,
    ZHPEU_THR_WAIT_SLEEP,
    ZHPEU_THR_WAIT_SIGNAL,
};

void zhpeu_thr_wait_init(struct zhpeu_thr_wait *thr_wait);
void zhpeu_thr_wait_destroy(struct zhpeu_thr_wait *thr_wait);

static inline bool zhpeu_thr_wait_signal_fast(struct zhpeu_thr_wait *thr_wait)
{
    int32_t             old = ZHPEU_THR_WAIT_IDLE;
    int32_t             new = ZHPEU_THR_WAIT_SIGNAL;

    /* One sleeper, many wakers. */
    if (atm_cmpxchg(&thr_wait->state, &old, new) || old == new)
        /* Done! */
        return false;

    /* Need slow path. */
    assert(old == ZHPEU_THR_WAIT_SLEEP);

    return true;
}

void zhpeu_thr_wait_signal_slow(struct zhpeu_thr_wait *thr_wait,
                                bool lock, bool unlock);

static inline void zhpeu_thr_wait_signal(struct zhpeu_thr_wait *thr_wait)
{
    if (zhpeu_thr_wait_signal_fast(thr_wait))
        zhpeu_thr_wait_signal_slow(thr_wait, true, true);
}

static inline bool zhpeu_thr_wait_sleep_fast(struct zhpeu_thr_wait *thr_wait)
{
    int32_t             old = ZHPEU_THR_WAIT_IDLE;
    int32_t             new = ZHPEU_THR_WAIT_SLEEP;

    /* One sleeper, many wakers. */
    if (atm_cmpxchg(&thr_wait->state, &old, new))
        /* Need to call slow. */
        return true;

    /* Reset SIGNAL to IDLE. */
    assert(old == ZHPEU_THR_WAIT_SIGNAL);
    new = ZHPEU_THR_WAIT_IDLE;
    atm_cmpxchg(&thr_wait->state, &old, new);

    /* Fast path succeeded. */
    return false;
}

int zhpeu_thr_wait_sleep_slow(struct zhpeu_thr_wait *thr_wait,
                              int64_t timeout_us, bool lock, bool unlock);

struct zhpeu_work_head {
    struct zhpeu_thr_wait thr_wait;
    STAILQ_HEAD(, zhpeu_work) work_list;
};

/* Worker returns true if it needs to be retried later. */
typedef bool (*zhpeu_worker)(struct zhpeu_work_head *head,
                             struct zhpeu_work *work);

struct zhpeu_work {
    STAILQ_ENTRY(zhpeu_work) lentry;
    zhpeu_worker        worker;
    void                *data;
    pthread_cond_t      cond;
    int                 status;
};

void zhpeu_work_head_init(struct zhpeu_work_head *head);
void zhpeu_work_head_destroy(struct zhpeu_work_head *head);

static inline void zhpeu_work_init(struct zhpeu_work *work)
{
    work->status = 0;
    work->worker = NULL;
    cond_init(&work->cond, NULL);
}

static inline void zhpeu_work_destroy(struct zhpeu_work *work)
{
    cond_destroy(&work->cond);
}

static inline void zhpeu_work_wait(struct zhpeu_work_head *head,
                                   struct zhpeu_work *work, bool lock,
                                   bool unlock)
{
    if (lock)
        mutex_lock(&head->thr_wait.mutex);
    while (work->worker)
        cond_wait(&work->cond, &head->thr_wait.mutex);
    if (unlock)
        mutex_unlock(&head->thr_wait.mutex);
}

static inline bool zhpeu_work_queued(struct zhpeu_work_head *head)
{
    return unlikely(!!STAILQ_FIRST(&head->work_list));
}

static inline void zhpeu_work_queue(struct zhpeu_work_head *head,
                                    struct zhpeu_work *work,
                                    zhpeu_worker worker, void *data,
                                    bool signal, bool lock, bool unlock)
{
    if (lock)
        mutex_lock(&head->thr_wait.mutex);
    work->worker = worker;
    work->data = data;
    STAILQ_INSERT_TAIL(&head->work_list, work, lentry);
    if (signal && zhpeu_thr_wait_signal_fast(&head->thr_wait))
        zhpeu_thr_wait_signal_slow(&head->thr_wait, false, unlock);
    else if (unlock)
        mutex_unlock(&head->thr_wait.mutex);
}

bool zhpeu_work_process(struct zhpeu_work_head *head, bool lock, bool unlock);

_EXTERN_C_END

#endif /* _ZHPEQ_UTIL_H_ */
