//
//  file.c
//  mds_proxy
//
//  Created by 周凯 on 2021/8/6.
//

#include "encode.h"

struct mds_file {
    bool closed; /*是否为闭合文件*/
    char name[FILE_NAME_SIZE];
    struct mds_istream istream;
};

static bool parse_file_name(char *path, struct file_name *fname)
{
    int l, n, v = 0;
    char *p2, *p1 = path;

    /*可能是 .. 或 .*/
    if (p1[0] == '.')
        return false;

    /*解析产品编号*/
    p2 = strchr(p1, '.');
    if (!p2)
        return false;

    p2[0] = '\0';
    while (p1[0] == '0') {
        p1++;
    }

    if (p1 != p2) {
        n = sscanf(p1, "%u%n", &v, &l);
        if (n != 1 || l != (int)(p2 - p1) || v < 0) {
            p2[0] = '.';
            return false;
        }
    }

    p2[0] = '.';
    fname->instrId = v;

    /*解析市场编码*/
    p1 = &p2[1];
    p2 = strchr(p1, '.');
    if (p2 - p1 != 2 || p1[0] != 'S')
        return false;
    if (p1[1] == 'H') {
        fname->exchId = MDSPXY_EXCH_SSE;
    } else if (p1[1] == 'Z') {
        fname->exchId = MDSPXY_EXCH_SZSE;
    } else {
        return false;
    }

    /*是否为闭合文件*/
    p1 = &p2[1];
    if (strcmp(p1, "tmp") == 0) {
        fname->seq = -1;
    } else {
        n = sscanf(p1, "%u%n", &v, &l);
        if (n != 1 || l != (int)strlen(p1) || v < 0)
            return false;
        fname->seq = v;
    }

    return true;
}

void get__commonInfo(const struct mkt_msg *msg, uint32_t *exchId,
        uint32_t *instrId, uint32_t *tradeDate)
{
    switch (msg->msgId) {
        case MDSPXY_MSGTYPE_L2_ORDER:
            if (exchId)
                *exchId = msg->order.exchId;
            if (instrId)
                *instrId = msg->order.instrId;
            if (tradeDate)
                *tradeDate = msg->order.tradeDate;
            break;
        case MDSPXY_MSGTYPE_L2_TRADE:
            if (exchId)
                *exchId = msg->trade.exchId;
            if (instrId)
                *instrId = msg->trade.instrId;
            if (tradeDate)
                *tradeDate = msg->trade.tradeDate;
            break;
        case MDSPXY_MSGTYPE_L2_MARKET_DATA_SNAPSHOT:
            if (exchId)
                *exchId = msg->snapshot.exchId;
            if (instrId)
                *instrId = msg->snapshot.instrId;
            if (tradeDate)
                *tradeDate = msg->snapshot.tradeDate;
            break;
        case MDSPXY_MSGTYPE_LOSTED_CHLSEQ:
            if (exchId)
                *exchId = msg->chlseq.exchId;
            if (instrId)
                *instrId = msg->chlseq.instrId;
            if (tradeDate)
                *tradeDate = msg->chlseq.tradeDate;
            break;
        default:
            abort();
    }
}

void get_common_info(const struct mds_data *data, uint32_t *exchId,
        uint32_t *instrId, uint32_t *tradeDate, uint64_t *seq)
{
    switch (data->type) {
        case MDSPXY_MSGTYPE_L2_ORDER:
            if (exchId)
                *exchId = data->l2_order.exchId;
            if (instrId)
                *instrId = data->l2_order.instrId;
            if (tradeDate)
                *tradeDate = data->l2_order.tradeDate;
            if (seq)
                *seq = data->l2_order.seq;
            break;
        case MDSPXY_MSGTYPE_L2_TRADE:
            if (exchId)
                *exchId = data->l2_trade.exchId;
            if (instrId)
                *instrId = data->l2_trade.instrId;
            if (tradeDate)
                *tradeDate = data->l2_trade.tradeDate;
            if (seq)
                *seq = data->l2_trade.seq;
            break;
        case MDSPXY_MSGTYPE_L2_MARKET_DATA_SNAPSHOT:
            if (exchId)
                *exchId = data->l2_snapshot.exchId;
            if (instrId)
                *instrId = data->l2_snapshot.instrId;
            if (tradeDate)
                *tradeDate = data->l2_snapshot.tradeDate;
            if (seq)
                *seq = data->l2_snapshot.seq;
            break;
        case MDSPXY_MSGTYPE_LOSTED_CHLSEQ:
            if (exchId)
                *exchId = data->chlseq.exchId;
            if (instrId)
                *instrId = data->chlseq.instrId;
            if (tradeDate)
                *tradeDate = data->chlseq.tradeDate;
            if (seq)
                *seq = data->chlseq.seq;
            break;
        default:
            abort();
    }
}

static bool init_mds_istream(int fd, struct mds_istream *stream)
{
    ssize_t rc;
    struct file_head fhead;

    rc = _pread(fd, &fhead, sizeof(fhead), 0);
    if (unlikely(rc < 0))
        return false;

    if (unlikely(rc != sizeof(fhead)))
        return false;

    fhead.magic = ntohl(fhead.magic);

    if (unlikely(fhead.magic != LOG_FILE_MAGIC))
        return false;

    fhead.version = ntohl(fhead.version);

    switch (fhead.version) {
        case LOG_FILE_VERSION_1:
            stream->read = read__mds_istream_v1;
            stream->lseek = lseek__mds_istream_v1;
            break;
        default:
            return false;
    }

    stream->fd = fd;
    stream->offset = sizeof(fhead);
    _lseek(fd, stream->offset, SEEK_SET);

    return true;
}

bool init__mds_istream(int fd, struct mds_istream *stream)
{
    memset(stream, 0, sizeof(*stream));
    if (init_mds_istream(fd, stream)) {
        stream->nr_msg = 0;
        INIT_LIST_HEAD(&stream->queue);
        return true;
    }
    return false;
}

static void finit__mds_istream(struct mds_istream *stream)
{
    _close(stream->fd);
    if (stream->nr_msg)
        free__mmsg_list(&stream->queue);
}

struct mds_file *mds_file_open(const char *path)
{
    int rc;
    char *p;
    struct file_name fname;
    struct mds_file *file = malloc(sizeof(*file));
    if (unlikely(!file)) {
        errno = ENOMEM;
        return NULL;
    }

    memset(file, 0, sizeof(*file));
    rc = snprintf(file->name, FILE_NAME_SIZE, "%s", path);
    if (unlikely(rc >= FILE_NAME_SIZE)) {
        errno = EINVAL;
        goto fail;
    }

    p = strrchr(file->name, '/');
    if (p) {
        p++;
    } else {
        p = file->name;
    }

    if (!parse_file_name(p, &fname)) {
        errno = EINVAL;
        goto fail;
    }

    file->closed = fname.seq < 0 ? false : true;

    rc = _open(path, _O_RDONLY);
    if (unlikely(rc < 0))
        goto fail;

    if (!init__mds_istream(rc, &file->istream)) {
        errno = EPROTO;
        goto fail;
    }

    return file;

fail:
    free(file);
    return NULL;
}

void mds_file_close(struct mds_file *file)
{
    if (unlikely(!file))
        return;

    finit__mds_istream(&file->istream);

    free(file);
    return;
}

static int mds_file_reopen(struct mds_file *file, int timeout)
{
    ssize_t rc = 0;
    struct mds_istream *stream = &file->istream;

    assert(!file->closed);

    do {
        rc = stream->read(stream);
        if (unlikely(rc < 0))
            return rc;
    } while (rc);

    _close(stream->fd);
    stream->fd = -1;

    /*尝试打开*/
    do {
        Sleep(10);
        rc = _open(file->name, _O_RDONLY);
        if (rc < 0) {
            if (unlikely(errno != ENOENT))
                break;
        }
    } while (rc < 0 && (timeout--) > 0);

    if (rc > -1) {
        do {
            Sleep(10);
            if (init_mds_istream(rc, stream))
                goto out;
        } while ((timeout--) > 0);
        return -ETIMEDOUT;
    }

out:
    if (list_empty(&stream->queue))
        return rc < 0 ? -ENOENT : 0;

    return 1;
}

int mds_file_read(struct mds_file *file, struct mds_data *data, int timeout)
{
    ssize_t rc;
    int tick = 5;
    uint64_t to = timeout;
    uint64_t start, escape;
    struct mkt_msg *msg;
    struct list_head *queue;
    struct mds_istream *stream;

    if (unlikely(!file))
        return -EINVAL;

    start = get_time();
    stream = &file->istream;
    queue = &stream->queue;

ready:
    if (stream->nr_msg) {
        msg = list_first_entry_or_null(queue, struct mkt_msg, q_node);
        assert(msg);

        stream->nr_msg--;
        mds_data_fill(data, msg);

        list_del(&msg->q_node);
        free(msg);

        return 1;
    }

again:
    rc = stream->read(stream);
    if (likely(rc > 0))
        goto ready;
    if (likely(rc == 0)) {
        if (file->closed)
            return 0;
    } else if (unlikely(rc != -EAGAIN)) {
        return rc;
    } else if (unlikely(file->closed)) {
        return -EPROTO;
    }

    if (!to)
        return -EAGAIN;

    escape = get_time() - start;
    if (escape >= to)
        return -ETIMEDOUT;

    if (tick>50)
        tick = 5;

    Sleep(tick++);

    /*判断是否更名*/ 
    if (!file->closed) {
        rc = get_filesize(file->name);
        if (rc > 0 && rc < file->istream.offset) {
            file->closed = true;
        }
    }

    goto again;
}

int mds_file_lseek(struct mds_file *file)
{
    ssize_t rc;
    struct mds_istream *stream;

    if (unlikely(!file))
        return -EINVAL;

    stream = &file->istream;

    if (stream->nr_msg) {
        free__mmsg_list(&stream->queue);
        stream->nr_msg = 0;
    }

    rc = stream->lseek(stream);
    if (unlikely(rc && rc != -EAGAIN))
        return rc;

    stream->nr_msg = 0;

    return 0;
}
