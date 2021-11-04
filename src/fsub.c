#include "encode.h"

struct subinfo {
    uint32_t exchId; /*为 0 表示没有使用*/
    uint32_t instrId;
    int32_t fseq; /*为-1表示已读取到最新的*/
    uint64_t dseq; /*数据序号*/
    struct mds_file *file; 
};

struct mds_fsub {
    /*订阅信息数组，用于记录补全状态，为0则表示没有使用该槽*/
    uint32_t vec_size;
    uint32_t nr_sub;
    uint32_t nr_incompleted;
    /*如果不为负数，表示正在读 vector 相应下表 subinfo.file 中数据*/
    int32_t read_file;
    struct subinfo *vector;

    struct mds_sub *sub;
    struct mds_data data; /*缓存来自订阅，但当前正在补全的数据*/

    char path[FILE_NAME_SIZE]; /*落地数据交易日目录所在的路径*/
};

extern int _mds_sub_recv(struct mds_sub *sub);

struct mds_fsub *mds_fsub_open(const char *addr, const char *path)
{
    int rc;
    struct mds_fsub *fsub;

    /*权限检查*/
    rc = _access(path, 0);
    if (unlikely(rc == -1))
        return NULL;

    fsub = malloc(sizeof(*fsub));
    if (unlikely(!fsub)) {
        errno = ENOMEM;
        return NULL;
    }

    memset(fsub, 0, sizeof(*fsub));

    fsub->read_file = -1;
    rc = snprintf(fsub->path, sizeof(fsub->path), "%s", path);
    if (rc >= sizeof(fsub->path)) {
        errno = EINVAL;
        goto fail;
    }

    fsub->sub = mds_sub_open(addr);
    if (unlikely(!fsub->sub))
        goto fail;

    return fsub;

fail:
    if (fsub->sub)
        mds_sub_close(fsub->sub);
    free(fsub);

    return NULL;
}

/*扩展*/
static int expand_subinfo(struct mds_fsub *fsub)
{
    struct subinfo *sinfo;
    uint32_t vec_size = fsub->vec_size;

    assert(vec_size >= fsub->nr_sub);

    if (vec_size != fsub->nr_sub)
        return 0;

    vec_size=vec_size?(vec_size*2):4; 
    sinfo = calloc(vec_size, sizeof(*sinfo));
    if (unlikely(!sinfo))
        return -ENOMEM;

    for (int i = 0; i < fsub->vec_size; i++) {
        assert(!sinfo[i].exchId);
        sinfo[i] = fsub->vector[i];
    }

    memset(&sinfo[fsub->vec_size], 0,
        sizeof(*sinfo) * (vec_size - fsub->vec_size));

    if (fsub->vector)
        free(fsub->vector);

    fsub->vector = sinfo;
    fsub->vec_size = vec_size;

    return 0;
}

/*收缩*/
static int shrink_subinfo(struct mds_fsub *fsub)
{
    int j = 0;
    struct subinfo *sinfo;
    uint32_t vec_size = fsub->vec_size;

    /*小于一半，则收缩*/
    if (fsub->nr_sub*2 > vec_size)
        return 0;

    if (vec_size < 9)
        return 0;

    sinfo = calloc(vec_size/2, sizeof(*sinfo));
    if (unlikely(!sinfo))
        return -ENOMEM;

    /*移除没有使用的*/
    for (int i = 0; i < vec_size; i++) {
        if (!fsub->vector[i].exchId)
            continue;
        memcpy(&sinfo[j++], &fsub->vector[i], sizeof(*sinfo));
    }

    while (j < vec_size/2)
        sinfo[j++].exchId = 0;

    free(fsub->vector);
    fsub->vector = sinfo;

    return 0;
}

static int subinfo_add(struct mds_fsub *fsub, const char *topic)
{
    struct subinfo sinfo;
    int rc, i, pos = U32_MAX;

    memset(&sinfo, 0, sizeof(sinfo));

    rc = parse_topic(topic, &sinfo.exchId, &sinfo.instrId);
    if (unlikely(rc))
        return rc;

    rc = expand_subinfo(fsub);
    if (unlikely(rc))
        return rc;

    for (i = 0; i < fsub->vec_size; i++) {

        if (pos == U32_MAX && !fsub->vector[i].exchId)
            pos = i; /*记录第一个空闲槽位*/
        if (fsub->vector[i].exchId == sinfo.exchId &&
                fsub->vector[i].instrId == sinfo.exchId)
            return -EEXIST;
    }

    assert(pos < fsub->vec_size);

    fsub->nr_sub++;
    fsub->nr_incompleted++;
    memcpy(&fsub->vector[pos], &sinfo, sizeof(sinfo));

    return 0;
}

static int subinfo_find(struct mds_fsub *fsub, uint32_t exchId,
        uint32_t instrId, struct subinfo **psinfo)
{
    if (unlikely(!fsub->nr_sub))
        return -ENOENT;

    for (int i = 0; i < fsub->vec_size; i++) {
        if (fsub->vector[i].exchId == exchId &&
                fsub->vector[i].instrId == instrId) {
            *psinfo = &fsub->vector[i];
            return 0;
        }
    }

    return -ENOENT;
}

static int subinfo_remove(struct mds_fsub *fsub, const char *topic)
{
    int rc;
    struct subinfo *sinfo;
    uint32_t exchId, instrId;

    rc = parse_topic(topic, &exchId, &instrId);
    if (unlikely(rc))
        return rc;

    rc = subinfo_find(fsub, exchId, instrId, &sinfo);
    if (unlikely(rc))
        return rc;
    
    fsub->nr_sub--;
    fsub->nr_incompleted--;

    /*查看是否是正在补全的数据*/
    if (fsub->read_file > -1) {
        uint32_t e, i;
        get_common_info(&fsub->data, &e, &i, 0, 0);
        if (exchId == e && instrId == i)
            fsub->read_file = -1;
    }

    sinfo->exchId = 0;
    if (sinfo->file) {
        mds_file_close(sinfo->file);
        sinfo->file = NULL;
    }

    /*收缩*/
    shrink_subinfo(fsub);

    return 0;
}

int mds_fsub_req(struct mds_fsub *fsub, const char *topic)
{
    int rc;
    if (unlikely(!fsub))
        return -EINVAL;
    rc = subinfo_add(fsub, topic);
    if (unlikely(rc))
        return rc;

    rc = mds_sub_req(fsub->sub, topic);
    if (unlikely(rc)) {
        int rc2 = subinfo_remove(fsub, topic);
        assert(!rc2);
        return rc;
    }

    return 0;
}

int mds_fsub_cancel(struct mds_fsub *fsub, const char *topic)
{
    int rc;

    rc = mds_sub_cancel(fsub->sub, topic);
    if (unlikely(rc))
        return rc;

    rc = subinfo_remove(fsub, topic);
    if (unlikely(rc))
        return rc;

    return 0;
}

static int open_file(struct subinfo *sinfo, const char *dir, uint32_t date,
        int *timeout)
{
    int tick = 5;
    char path[FILE_NAME_SIZE];
    uint64_t start = get_time();
    uint64_t to = *timeout < 0?INT64_MAX:*timeout;

again:
    _format_seqfile(path, dir, date, sinfo->exchId, sinfo->instrId,
            sinfo->fseq);

    /*尝试打开闭合文件*/
    sinfo->file = mds_file_open(path);
    if (unlikely(!sinfo->file)) {
        if (errno != ENOENT)
            goto out;

        /*尝试打开非闭合文件*/
        _format_tmpfile(path, dir, date, sinfo->exchId, sinfo->instrId);
        sinfo->file = mds_file_open(path);
        if (sinfo->file || errno != ENOENT) {
            goto out;
        }

        if (!to) {
            errno = EAGAIN;
            goto out;
        }

        /*查看是否超时*/
        if (get_time() >= start + to) {
            errno = ETIMEDOUT;
            goto out;
        }

        /*休眠一会儿*/
        if (tick > 50)
            tick = 5;

        Sleep(tick++);
        /*再次尝试打开闭合文件*/
        goto again;             
    }

    sinfo->fseq++;

out:
    if (to && to != INT64_MAX)
        *timeout = (int32_t)(to - (get_time() - start));
    if (sinfo->file)
        return 0;
    return -errno;
}

static void mds_fsub_close_file(struct mds_fsub *fsub)
{
    struct subinfo *sinfo = NULL;
    assert(fsub->read_file > -1);
    sinfo = &fsub->vector[fsub->read_file];

    mds_file_close(sinfo->file);
    sinfo->fseq = -1;
    sinfo->file = NULL;

    fsub->read_file = -1;
    fsub->nr_incompleted--;
}

static int mds_fsub_read_file(struct mds_fsub *fsub, struct mds_data *data,
        int timeout)
{
    int rc;
    uint64_t seq1, seq2;
    struct mds_data tmp;
    uint32_t tradeDate = 0;
    struct subinfo *sinfo = NULL;

    assert(fsub->read_file > -1);

    if (unlikely(!data))
        data = &tmp;

    sinfo = &fsub->vector[fsub->read_file];
    if (unlikely(!sinfo->file)) {
next:
        get_common_info(&fsub->data, 0, 0, &tradeDate, 0);
        rc = open_file(sinfo, fsub->path, tradeDate, &timeout);
        if (unlikely(rc))
            return rc;
    }

    rc = mds_file_read(sinfo->file, data, timeout);
    if (unlikely(rc < 0))
        return rc;

    /*闭合文件读取完毕了*/
    if (unlikely(!rc)) {
        mds_file_close(sinfo->file);
        sinfo->file = NULL;
        goto next;
    }

    get_common_info(data, 0, 0, 0, &seq1);
    /*
     *可能遭遇了服务器重命名非闭合文件和打开新的非闭合文件文件
     *但是订阅先发送，不可能出现这样的情况？
     */
    if (unlikely(sinfo->dseq + 1 != seq1))
        return -EPROTO;

    get_common_info(&fsub->data, 0, 0, 0, &seq2);
    if (unlikely(seq1 >= seq2)) {
        /*文件中的数据不连续*/
        if (unlikely(seq1 > seq2 &&
                data->type != MDSPXY_MSGTYPE_LOSTED_CHLSEQ))
            return -EPROTO;

        mds_fsub_close_file(fsub);
        /*读取缓存的数据*/
        memcpy(data, &fsub->data, sizeof(*data));
    }

    sinfo->dseq = seq1;
    /*继续接收订阅，防止丢失*/
    _mds_sub_recv(fsub->sub);

    return 0;
}

int mds_fsub_recv(struct mds_fsub *fsub, struct mds_data *data, int timeout)
{
    int rc;
    uint64_t seq = 0;
    struct subinfo *sinfo = NULL;
    uint32_t exchId = 0, instrId = 0;

    if (unlikely(!fsub))
        return -EINVAL;

    /*查看是否正常处理文件补全订阅*/
    if (fsub->read_file > -1) {
rfile:
        rc = mds_fsub_read_file(fsub, data, timeout);
        /*查看是否是心跳信息，是则忽略*/
        if (likely(!rc && data->type == MDSPXY_MSGTYPE_LOSTED_CHLSEQ &&
                data->chlseq.StartApplSeqNum == -1))
            goto next;
        return rc;
    }

next:
    /*接收订阅*/
    rc = mds_sub_recv(fsub->sub, &fsub->data, timeout);
    if (unlikely(rc))
        return rc;

    get_common_info(&fsub->data, &exchId, &instrId, 0, &seq);

    /*判断是否仍有待补全的订阅信息*/
    if (unlikely(seq == 1) || !fsub->nr_incompleted) {
out:
        if (data)
            memcpy(data, &fsub->data, sizeof(*data));
        return 0;
    }

    /*查找订阅信息*/
    rc = subinfo_find(fsub, exchId, instrId, &sinfo);
    assert(!rc);

    /*此股票已经完成补全*/
    if (sinfo->fseq == -1)
        goto out;

    /*从第一个闭合文件开始读取*/
    sinfo->fseq = 0;
    /*标记开始读文件*/
    fsub->read_file = (int32_t)(sinfo - fsub->vector); 

    goto rfile;
}

void mds_fsub_close(struct mds_fsub *fsub)
{
    if (unlikely(!fsub))
        return;

    for (int i = 0; i < fsub->vec_size; i++) {
        if (fsub->vector[i].exchId) {
            mds_file_close(fsub->vector[i].file);
        }
    }

    if (fsub->vector)
        free(fsub->vector);
    mds_sub_close(fsub->sub);
    free(fsub);
}
