//
//  read_v1.c
//  mds_proxy
//
//  Created by 周凯 on 2021/8/6.
//

#include "encode.h"
#include "data.h"

static ssize_t read_nointr(int fd, void *ptr, size_t l)
{
    ssize_t rc = 0;
    do {
        rc = _read(fd, ptr, l);
    } while (rc < 0 && errno == EINTR);
    return rc;
}

static ssize_t readv_nointr(int fd, struct _iovec *iov, int n)
{
    ssize_t l = 0, rc = 0;
    for (int i = 0; i < n; i++) {
        rc = read_nointr(fd, iov[i].iov_base, iov[i].iov_len);
        if (rc < 1)
            return rc;
        l += rc;
    }
    return l;
}

static ssize_t lseek_mds_istream_v1(struct mds_istream *stream)
{
    ssize_t rc;
    uint32_t nr;
    size_t ll = 0;
    char data[1024];
    char *buff = NULL;
    struct batch_head bh;
    struct segment_head *sh = NULL;
    off_t offset = stream->offset;

    rc = read_nointr(stream->fd, &bh, sizeof(bh));
    if (unlikely(rc < 0)) {
        return -errno;
    }
    if (unlikely(rc != sizeof(bh))) {
        rc = rc?-EAGAIN:0;
        goto out;
    }

    offset += sizeof(bh);
    nr = ntohl(bh.nr_payload);
    if (unlikely(!nr)) {
        if (unlikely(bh.body_checksum)) {
            rc = -EPROTO;
            goto out;
        }
        goto end;
    }

    ll = nr * sizeof(*sh);
    sh = malloc(ll);
    if (unlikely(!sh)) {
        rc = -ENOMEM;
        goto out;
    }

    rc = read_nointr(stream->fd, sh, ll);
    if (unlikely(rc < 0)) {
        rc = -errno;
        goto out;
    }
    if (unlikely(rc != ll)) {
        rc = -EAGAIN;
        goto out;
    }

    offset += ll;

    ll = 0;
    for (uint32_t i = 0; i < nr; i++) {
        ll += ntohl(sh[i].length);
    }

    if (ll > sizeof(data)) {
        buff = malloc(ll);
        if (unlikely(!buff)) {
            rc = -ENOMEM;
            goto out;
        }
    } else {
        buff = data;
    }

    rc = read_nointr(stream->fd, buff, ll);
    if (unlikely(rc < 0)) {
        rc = -errno;
        goto out;
    }
    if (unlikely(rc != ll)) {
        rc = -EAGAIN;
        goto out;
    }

    offset += ll;

end:
    rc = offset - stream->offset;
    stream->nr_msg += nr;
    stream->offset = offset;

out:
    if (sh)
        free(sh);
    if (buff && buff != data)
        free(buff);

    /*数据不够或数据一致性错误则还原偏移*/
    if (unlikely(rc < 1)) {
        _lseek(stream->fd, stream->offset, SEEK_SET);
        errno = -rc;
    }

    return rc;
}

ssize_t lseek__mds_istream_v1(struct mds_istream *stream)
{
    ssize_t rc = 0;

    do {
        rc = lseek_mds_istream_v1(stream);
    } while (rc > 0);

    return rc;
}

ssize_t read__mds_istream_v1(struct mds_istream *stream)
{
    ssize_t rc;
    uint32_t nr;
    size_t ll = 0;
    uint32_t sum1, sum2;
    struct batch_head bh;
    struct list_head queue;
    struct mkt_msg *msg = NULL;
    struct _iovec *iovec = NULL;
    struct segment_head *sh = NULL;
    off_t offset = stream->offset;

    INIT_LIST_HEAD(&queue);

    rc = read_nointr(stream->fd, &bh, sizeof(bh));
    if (unlikely(rc < 0)) {
        return -errno;
    }
    if (unlikely(rc != sizeof(bh))) {
        rc = rc?-EAGAIN:0;
        goto out;
    }

    offset += sizeof(bh);
    nr = ntohl(bh.nr_payload);
    if (unlikely(!nr)) {
        if (unlikely(bh.body_checksum)) {
            rc = -EPROTO;
            goto out;
        }
        sum2 = ntohl(bh.hdr_checksum);
        bh.hdr_checksum = 0;
        sum1 = byteCrc32(&bh, sizeof(bh), 0);
        if (unlikely(sum1 != sum2)) {
            rc = -EPROTO;
            goto out;
        }
        goto end;
    }

    ll = nr * sizeof(*sh);
    sh = malloc(ll);
    if (unlikely(!sh)) {
        rc = -ENOMEM;
        goto out;
    }

    rc = read_nointr(stream->fd, sh, ll);
    if (unlikely(rc < 0)) {
        rc = -errno;
        goto out;
    }
    if (unlikely(rc != ll)) {
        rc = -EAGAIN;
        goto out;
    }

    offset += ll;
    sum2 = ntohl(bh.hdr_checksum);

    bh.hdr_checksum = 0;
    sum1 = byteCrc32(&bh, sizeof(bh), 0);
    sum1 = byteCrc32(sh, ll, sum1);
    if (unlikely(sum1 != sum2)) {
        rc = -EPROTO;
        goto out;
    }

    iovec = calloc(nr, sizeof(*iovec));
    if (unlikely(!iovec)) {
        rc = -ENOMEM;
        goto out;
    }

    ll = 0;
    for (uint32_t i = 0; i < nr; i++) {
        ssize_t l = 0;

        msg = malloc(sizeof(*msg));
        if (unlikely(!msg)) {
            rc = -ENOMEM;
            goto out;
        }

        memset(msg, 0, sizeof(*msg));

        iovec[i].iov_base = msg->data;
        iovec[i].iov_len = ntohl(sh[i].length);

        list_add_tail(&msg->q_node, &queue);
        msg->msgId = ntohl(sh[i].type);

        l = mmsg_size(msg);
        if (unlikely(l < 0)) {
            rc = -EPROTO;
            goto out;
        }

        ll += l;
        if (l != iovec[i].iov_len) {
            rc = -EPROTO;
            goto out;
        }
    }

    rc = readv_nointr(stream->fd, iovec, nr);
    if (unlikely(rc < 0)) {
        rc = -errno;
        goto out;
    }
    if (unlikely(rc != ll)) {
        rc = -EAGAIN;
        goto out;
    }

    offset += ll;
    decode_body(&queue, &sum1);
    sum2 = ntohl(bh.body_checksum);
    if (unlikely(sum2 != sum1)) {
        rc = -EPROTO;
        goto out;
    }

end:
    rc = offset - stream->offset;
    stream->nr_msg += nr;
    stream->offset = offset;
    list_splice_tail_init(&queue, &stream->queue);

out:
    if (sh)
        free(sh);
    if (iovec)
        free(iovec);
    free__mmsg_list(&queue);

    /*数据不够或数据一致性错误则还原偏移*/
    if (unlikely(rc < 1)) {
        _lseek(stream->fd, stream->offset, SEEK_SET);
        errno = -rc;
    }

    return rc;
}
