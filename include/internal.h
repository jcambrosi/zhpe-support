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

#ifndef _LIBZHPEQ_INTERNAL_H_
#define _LIBZHPEQ_INTERNAL_H_

#include <zhpeq.h>
#include <zhpeq_util.h>
#include <zhpe.h>
#include <zhpe_stats.h>

#include <assert.h>
#include <endian.h>
#include <inttypes.h>

#include <uuid/uuid.h>

_EXTERN_C_BEG

#define DEV_NAME        "/dev/"DRIVER_NAME

struct key_data_packed;
struct zhpeq_domi;
struct zhpeq_rqi;
struct zhpeq_xqi;

struct backend_ops {
    int                 (*lib_init)(struct zhpeq_attr *attr);
    int                 (*domain)(struct zhpeq_domi *zdomi);
    int                 (*domain_free)(struct zhpeq_domi *zdomi);
    int                 (*xqalloc)(struct zhpeq_xqi *xqi,
                                   int cmd_qlen, int cmp_qlen,
                                   int traffic_class, int priority,
                                   int slice_mask);
    int                 (*xqalloc_post)(struct zhpeq_xqi *xqi);
    int                 (*xqfree_pre)(struct zhpeq_xqi *xqi);
    int                 (*xqfree)(struct zhpeq_xqi *xqi);
    int                 (*open)(struct zhpeq_xqi *xqi, void *sa);
    int                 (*close)(struct zhpeq_xqi *xqi, int open_idx);
    int                 (*wq_signal)(struct zhpeq_xqi *zxq);
    ssize_t             (*cq_poll)(struct zhpeq_xqi *xqi, size_t len);
    int                 (*mr_reg)(struct zhpeq_domi *zdomi,
                                  const void *buf, size_t len, uint32_t access,
                                  struct zhpeq_key_data **qkdata_out);
    int                 (*mr_free)(struct zhpeq_key_data *qkdata);
    int                 (*qkdata_export)(const struct zhpeq_key_data *qkdata,
                                         struct key_data_packed *blob);
    int                 (*zmmu_reg)(struct zhpeq_key_data *qkdata);
    int                 (*zmmu_free)(struct zhpeq_key_data *qkdata);
    int                 (*fam_qkdata)(struct zhpeq_dom *zdom, int open_idx,
                                      struct zhpeq_key_data **qkdata_out);
    int                 (*mmap)(const struct zhpeq_key_data *qkdata,
                                uint32_t cache_mode, void *addr,
                                size_t length, int prot, int flags,
                                off_t offset,
                                struct zhpeq_mmap_desc **zmdesc_out);
    int                 (*mmap_unmap)(struct zhpeq_mmap_desc *zmdesc);
    int                 (*mmap_commit)(struct zhpeq_mmap_desc *zmdesc,
                                       const void *addr, size_t length,
                                       bool fence, bool invalidate, bool wait);
    void                (*print_xq_info)(struct zhpeq_xq *zxq);
    int                 (*xq_get_addr)(struct zhpeq_xq *zxq, void *sa,
                                    size_t *sa_len);
    char                *(*qkdata_id_str)(const struct zhpeq_key_data *qkdata);
};

extern void             (*zhpeq_mcommit)(void);
extern uuid_t           zhpeq_uuid;

void zhpeq_register_backend(enum zhpe_backend backend, struct backend_ops *ops);
void zhpeq_backend_libfabric_init(int fd);
void zhpeq_backend_zhpe_init(int fd);

#define FREE_END        ((intptr_t)-1)

struct free_index {
    int32_t             index;
    uint32_t            seq;
} INT64_ALIGNED;

struct zhpeq_ht {
    uint32_t            head;
    uint32_t            tail;
} INT64_ALIGNED;

struct zhpeq_domi {
    struct zhpeq_dom    pub;
    void                *backend_data;
};

struct zhpeq_xqi {
    struct zhpeq_xq     pub;
    void                *backend_data;
    uint64_t            *free_bitmap;
    int                 fd;
};

struct zhpeq_rqi {
    struct zhpeq_rq     pub;
};

static inline uint8_t cq_valid(uint32_t idx, uint32_t qmask)
{
    return ((idx & (qmask + 1)) ? 0 : ZHPE_HW_CQ_VALID);
}

#define ZHPEQ_MR_VALID_MASK \
    (ZHPE_MR_GET | ZHPE_MR_PUT | ZHPE_MR_SEND | ZHPE_MR_RECV | \
     ZHPE_MR_GET_REMOTE | ZHPE_MR_PUT_REMOTE | \
     ZHPE_MR_FLAG0 | ZHPE_MR_FLAG1 | ZHPE_MR_FLAG2)

struct key_data_packed {
    uint64_t            vaddr;
    uint64_t            zaddr;
    uint64_t            len;
    uint8_t             access;
} __attribute__((packed));

static inline void pack_kdata(const struct zhpeq_key_data *qkdata,
                              struct key_data_packed *pdata,
                              uint64_t zaddr)
{
    const struct zhpe_key_data *kdata = &qkdata->z;

    pdata->vaddr = be64toh(kdata->vaddr);
    pdata->zaddr = be64toh(zaddr);
    pdata->len = be64toh(kdata->len);
    pdata->access = kdata->access;
}

static inline void unpack_kdata(const struct key_data_packed *pdata,
                                struct zhpeq_key_data *qkdata)
{
    struct zhpe_key_data *kdata = &qkdata->z;

    kdata->vaddr = htobe64(pdata->vaddr);
    kdata->zaddr = htobe64(pdata->zaddr);
    kdata->len = htobe64(pdata->len);
    kdata->access = pdata->access;
}

#define ZHPEQ_MR_V1             (1U)
#define ZHPEQ_MR_REMOTE         ((uint32_t)1 << 31)

struct zhpeq_mr_desc_common_hdr {
    uint32_t            magic;
    uint32_t            version;
    struct zhpeq_domi   *zdomi;
};

struct zhpeq_mr_desc_v1 {
    struct zhpeq_mr_desc_common_hdr hdr;
    struct zhpeq_key_data qkdata;
    int                 open_idx;
};

union zhpeq_mr_desc {
    struct zhpeq_mr_desc_common_hdr hdr;
    struct zhpeq_mr_desc_v1 v1;
};

struct zhpeq_mmap_desc_private {
    struct zhpeq_mmap_desc pub;
    struct zhpeq_mr_desc_v1 *desc;
};

/* FIXME: probably works for now, but ditch bit fields. */
union xdm_cmp_tail {
    struct zhpe_xdm_cmpl_queue_tail_toggle bits;
    uint64_t            u64;
};

union xdm_active {
    struct zhpe_xdm_active_status_error bits;
    uint64_t            u64;
};

union rdm_rcv_tail {
    struct zhpe_rdm_rcv_queue_tail_toggle bits;
    uint64_t            u64;
};

_EXTERN_C_END

#endif /* _LIBZHPEQ_INTERNAL_H */
