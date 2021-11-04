#ifndef __encode_h__
#define __encode_h__

#include "data.h"

__BEGIN_DECLS

#define LOG_FILE_MAGIC 0xdeadbeefU

#define LOG_FILE_VERSION_1 1

/*
 * 文件中包含多个这样的片段
 * 每个片段最多包含64个对象
 * magic|version|hdr_checksum|body_checksum|nr_payload
 * |[type,length][type,length][type,length]...
 * |[obj][obj]....|
 */
struct file_head {
    uint32_t magic;
    uint32_t version;
};

struct segment_head;

struct segment_head {
    uint32_t type;
    uint32_t length;
};

struct batch_head {
    uint32_t hdr_checksum;
    uint32_t body_checksum;
    uint32_t nr_payload;
    uint32_t __pad;
    struct segment_head seg_head[0];
};

union segment_body {
    struct losted_chlseq chlseq;
    struct local_l2order order;
    struct local_l2trade trade;
    struct local_l2snapshot snapshot;
    /* ... */
    char data[1];
};

void encode_chlseq(const struct losted_chlseq *src, struct losted_chlseq *dst);
void encode_l2order(const struct local_l2order *src, struct local_l2order *dst);
void encode_l2trade(const struct local_l2trade *src, struct local_l2trade *dst);
void encode_l2snapshot(const struct local_l2snapshot *src,
        struct local_l2snapshot *dst);

void decode_chlseq(const struct losted_chlseq *src, struct losted_chlseq *dst);
void decode_l2order(const struct local_l2order *src, struct local_l2order *dst);
void decode_l2trade(const struct local_l2trade *src, struct local_l2trade *dst);
void decode_l2snapshot(const struct local_l2snapshot *src,
        struct local_l2snapshot *dst);

/*计算编码头部需要的空间*/

void decode_body(struct list_head *, uint32_t *);
void encode_head(struct list_head *, uint32_t, struct batch_head *);
size_t encode_body(struct list_head *, uint32_t *, struct _iovec *);

static inline size_t head_size(uint32_t nr)
{
    return sizeof(struct batch_head) + sizeof(struct segment_head) * nr;
}

ssize_t body_size(struct list_head *);

__END_DECLS

#endif
