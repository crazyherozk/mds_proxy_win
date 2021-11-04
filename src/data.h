#ifndef __data_h__
#define __data_h__

#include "list.h"
#include "utils.h"
#include <mdspxy_api/mdspxy.h>

__BEGIN_DECLS

#define FILE_NAME_SIZE            128
/*缺失通道序号使用一个不存在的产品号来命名文件，以统一文件读写方式*/
#define LOSTED_CHLSEQ_INSTRID     0

struct file_name {
    uint32_t exchId;
    uint32_t instrId;
    int32_t seq; /**< 小于0表示为 临时文件*/
};

struct mds_istream {
    int32_t          fd;
    uint32_t         nr_msg;
    struct list_head queue;
    off_t            offset;
    ssize_t          (*lseek)(struct mds_istream*); /**< 移动到最后一条*/
    ssize_t          (*read)(struct mds_istream*); /**<
     * 因版本不同而不同
     * @return 0 在读取完整批次后到达文件尾
     *         -EAGAIN 在未读取完整批次后到达文件尾，如果是非闭合文件则可以重试
     *         -ENOMEM 内存不足
     *         -EPROTO 读取数据一致性错误，必须放弃读取后续数据
     *         <0 其他调用 read()/readv() 时的错误，应该不会出现这样的错误
     */
};

/*推送的行情信息*/
struct mkt_msg {
    uint32_t msgId;/**< 消息代码 @see eMdsMsgTypeT*/
    struct list_head q_node; /**< 排队待处理*/
    union {
        struct losted_chlseq chlseq;
        /* L2 逐笔委托行情 */
        struct local_l2order order;
        /* L2 逐笔成交行情 */
        struct local_l2trade trade;
        /* L2 快照 */
        struct local_l2snapshot snapshot;
        /* 数据起始 */
        char data[1];
    };
};

static inline ssize_t mmsg_size(const struct mkt_msg *msg)
{
    ssize_t l = 0;
    switch (msg->msgId) {
        case MDSPXY_MSGTYPE_L2_ORDER:
            l = sizeof(msg->order);
            break;
        case MDSPXY_MSGTYPE_L2_TRADE:
            l = sizeof(msg->trade);
            break;
        case MDSPXY_MSGTYPE_L2_MARKET_DATA_SNAPSHOT:
            l = sizeof(msg->snapshot);
            break;
        case MDSPXY_MSGTYPE_LOSTED_CHLSEQ:
            l = sizeof(msg->chlseq);
            break;
        default:
            return -EPROTO;
    }
    return l;
}

static inline void mds_data_fill(struct mds_data *data,
        const struct mkt_msg *msg)
{
    if (likely(data)) {
        size_t l = mmsg_size(msg);
        data->type = (uint32_t)msg->msgId;
        assert(l > 0);
        memcpy(data->data, msg->data, l);
    }
}

int parse_topic(const char *topic, uint32_t *exchId, uint32_t *instrId);

void free__mmsg_list(struct list_head *head);

void get__commonInfo(const struct mkt_msg*, uint32_t*, uint32_t*, uint32_t *);

bool init__mds_istream(int fd, struct mds_istream *);
/*移动到最后一条以读取最新的数据*/
ssize_t lseek__mds_istream_v1(struct mds_istream *);
ssize_t read__mds_istream_v1(struct mds_istream *);

static inline const char *exchIdstr(uint32_t exchId)
{
    static __thread char b[3];
    b[0] = 'S';
    if (likely(exchId == MDSPXY_EXCH_SSE)) {
        b[1] = 'H';
    } else if (likely(exchId == MDSPXY_EXCH_SZSE)) {
        b[1] = 'Z';
    } else {
        abort();
    }
    return b;
}

static inline void _format_tmpfile(char *b, const char *dir, uint32_t d,
        uint32_t e, uint32_t i)
{
    snprintf(b, FILE_NAME_SIZE, "%s/%d/%06u.%s.tmp", dir, d, i, exchIdstr(e));
}

static inline void _format_seqfile(char *b, const char *dir, uint32_t d,
        uint32_t e, uint32_t i, uint32_t s)
{
    snprintf(b, FILE_NAME_SIZE, "%s/%d/%06u.%s.%u", dir, d, i, exchIdstr(e), s);
}

__END_DECLS

#endif
