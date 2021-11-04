#include <zmq/zmq.h>
#include "list.h"
#include "encode.h"

#define NR_CACHE 1024*8 /*订阅缓存数*/

static void *zmq_ctx;
typedef CRITICAL_SECTION pthread_mutex_t;
#define PTHREAD_MUTEX_INITIALIZER {(void*)-1,-1,0,0,0,0}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int pthread_mutex_lock(pthread_mutex_t *m)
{
	EnterCriticalSection(m);
	return 0;
}

static int pthread_mutex_unlock(pthread_mutex_t *m)
{
	LeaveCriticalSection(m);
	return 0;
}

int _mds_sub_recv(struct mds_sub *sub);

struct mds_sub {
    void *zmq_sub;
    struct list_head queue; /*缓存的消息*/
    uint32_t nr_msg;
    uint32_t step;
    char topic[32]; /*接收topic缓存*/
    uint16_t msgId;
    uint16_t msglen;
    uint32_t checksum;
};

static void clear_ctx(void)
{
    pthread_mutex_lock(&mutex);
    if (zmq_ctx) {
        zmq_ctx_destroy(zmq_ctx);
        zmq_ctx = NULL;
    }
    pthread_mutex_unlock(&mutex);
}

struct mds_sub *mds_sub_open(const char *addr)
{
    int rc;
    void *ctx;
    struct mds_sub *sub;
    
    assert(addr);

    READ_ONCE(zmq_ctx, ctx);

    if (unlikely(!ctx)) {
        pthread_mutex_lock(&mutex);
        if (!zmq_ctx) {
            zmq_ctx = zmq_ctx_new();
            if (unlikely(!zmq_ctx)) {
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            atexit(clear_ctx);
        }
        pthread_mutex_unlock(&mutex);
        READ_ONCE(zmq_ctx, ctx);
    }

    sub = malloc(sizeof(*sub));
    if (unlikely(!sub)) {
        errno = ENOMEM;
        return NULL;
    }

    memset(sub, 0, sizeof(*sub));

    sub->zmq_sub = zmq_socket(ctx, ZMQ_SUB);
    if (unlikely(!sub->zmq_sub)) {
        rc = errno;
        goto fail;
    }

    rc = zmq_connect(sub->zmq_sub, addr);
    if (unlikely(rc != 0)) {
        rc = errno;
        goto fail;
    }

    INIT_LIST_HEAD(&sub->queue);

    return sub;

fail:
    if (sub->zmq_sub)
        zmq_close(sub->zmq_sub);

    free(sub);

    return NULL;
}

static int _mds_sub_req(struct mds_sub *sub, int type, const char *topic)
{
    int rc;
    uint32_t exchId, instrId;

    if (unlikely(!sub || !topic))
        return -EINVAL;

    rc = parse_topic(topic, &exchId, &instrId);
    if (unlikely(rc))
        return -EINVAL;

    rc = zmq_setsockopt(sub->zmq_sub, type, topic, strlen(topic));
    if (unlikely(rc != 0))
        return -errno;
    return rc;
}

int mds_sub_req(struct mds_sub *sub, const char *topic)
{
    return _mds_sub_req(sub, ZMQ_SUBSCRIBE, topic);
}

static int __mds_sub_recv(struct mds_sub *sub)
{
    ssize_t l;
    int more;
    uint64_t head;
    struct mkt_msg payload, *mmsg;
    static const char tmp[] = "000000.XX";

    /*三帧数据构成一个订阅数据*/
    switch (sub->step) {
        case 0:
        {
            l = zmq_recv(sub->zmq_sub, sub->topic, sizeof(sub->topic),
                         ZMQ_NOBLOCK);
            if (l == -1)
                return -errno;
            if (unlikely(l != sizeof(tmp) - 1))
                return -EPROTO;
            /*TODO:解析股票代码和市场代码，然后与负载对比*/
            l = sizeof(more);
            l = zmq_getsockopt(sub->zmq_sub, ZMQ_RCVMORE, &more, (size_t*)&l);
            if (unlikely(l == -1))
                return -errno;
            if (unlikely(!more))
                return -EPROTO;
            sub->step = 1;
        }
        case 1:
        {
            l = zmq_recv(sub->zmq_sub, &head, sizeof(head), ZMQ_NOBLOCK);
            if (l == -1)
                return -errno;
            if (unlikely(l != sizeof(head)))
                return -EPROTO;

            l = sizeof(more);
            l = zmq_getsockopt(sub->zmq_sub, ZMQ_RCVMORE, &more, (size_t*)&l);
            if (unlikely(l == -1))
                return -errno;
            if (unlikely(!more))
                return -EPROTO;

            /*低32位为检验和，接下来16位为负载长度，接下来16位为消息ID*/
            head = ntohll(head);
            sub->checksum = (uint32_t)(head & UINT32_MAX);
            sub->msglen = (uint16_t)((head >> 32) & UINT16_MAX);
            sub->msgId = (uint16_t)((head >> 48) & UINT16_MAX);

            payload.msgId = sub->msgId;
            l = mmsg_size(&payload);
            if (unlikely(l < 0 || l != sub->msglen))
                return -EPROTO;

            sub->step = 2;
        }
        case 2:
        {
            uint32_t checksum;
            struct list_head list;
            l = zmq_recv(sub->zmq_sub, payload.data, sub->msglen, ZMQ_NOBLOCK);
            if (l == -1)
                return -errno;
            if (unlikely(l != sub->msglen))
                return -EPROTO;

            l = sizeof(more);
            l = zmq_getsockopt(sub->zmq_sub, ZMQ_RCVMORE, &more, (size_t*)&l);
            if (unlikely(l == -1))
                return -errno;
            if (unlikely(more))
                return -EPROTO;

            INIT_LIST_HEAD(&list);
            list_add(&payload.q_node, &list);
            payload.msgId = sub->msgId;
            decode_body(&list, &checksum);
            if (unlikely(checksum != sub->checksum))
                return -EPROTO;

            mmsg = malloc(sizeof(*mmsg));
            if (unlikely(!mmsg))
                return -ENOMEM;

            memcpy(mmsg, &payload, sizeof(*mmsg));
            INIT_LIST_HEAD(&mmsg->q_node);

            sub->step = 0;
            sub->nr_msg++;
            list_add_tail(&mmsg->q_node, &sub->queue);
        }
            break;
        default:
            assert(0);
    }

    return 0;
}

int mds_sub_cancel(struct mds_sub *sub, const char *topic)
{
    int rc;
    struct mkt_msg *mmsg, *tmp;

    rc = _mds_sub_req(sub, ZMQ_UNSUBSCRIBE, topic);
    if (unlikely(rc))
        return rc;

    /*继续接收直到没有数据，然后删除所有缓存的匹配数据*/
    while (__mds_sub_recv(sub) == 0) { }

    list_for_each_entry_safe(mmsg, tmp, &sub->queue, struct mkt_msg, q_node) {
        char buff[32];
        uint32_t instId, exchId;

        get__commonInfo(mmsg, &exchId, &instId, 0);
        snprintf(buff, sizeof(buff), "%06d.%s", instId, exchIdstr(exchId));

        if (strcmp(buff, topic))
            continue;

        sub->nr_msg--;
        list_del(&mmsg->q_node);
        free(mmsg);
    }

    return 0;
}

int _mds_sub_recv(struct mds_sub *sub)
{
    int rc;

    if (sub->nr_msg >= NR_CACHE)
        return 0;

    do {
        rc = __mds_sub_recv(sub);
    } while (rc == 0 && sub->nr_msg < NR_CACHE);

    return rc;
}

/*弹出一条数据*/
static inline bool pop_mds_data(struct mds_sub *sub, struct mds_data *data)
{
    struct mkt_msg *mmsg;
    while (sub->nr_msg) {
        assert(!list_empty(&sub->queue));
        mmsg = list_first_entry(&sub->queue, struct mkt_msg, q_node);
        mds_data_fill(data, mmsg);

        sub->nr_msg--;
        list_del(&mmsg->q_node);
        free(mmsg);

        return true;
    }
    return false;
}

int mds_sub_recv(struct mds_sub *sub, struct mds_data *data, int timeout)
{
    int rc;
    uint64_t to;
    uint64_t start, escape;
    zmq_pollitem_t pollitem;

    if (unlikely(!sub))
        return -EINVAL;

    start = get_time();
    to = timeout;
    if (timeout < 0)
        to = UINT64_MAX;

again:
    /*每次都接收一些数据，防止订阅数据被服务端丢弃*/
    rc = _mds_sub_recv(sub);

    /*如果出错立马报告，而不管是否缓存数据?*/
    if (rc < 0 && rc != -EAGAIN)
        return rc;

    /*弹出一条*/    
    if (pop_mds_data(sub, data))
        return 0;

    if (timeout == 0)
        return -EAGAIN;

    /*慢速路径*/
    pollitem.fd = -1;
    pollitem.events = ZMQ_POLLIN;
    pollitem.socket = sub->zmq_sub;

    rc = zmq_poll(&pollitem, 1, 64);
    if (unlikely(rc == -1))
        return -errno;

    if (rc == 0) {
        escape = get_time() - start;
        if (to <= escape)
            return -ETIMEDOUT;
    }
    goto again;
}

void mds_sub_close(struct mds_sub *sub)
{
    struct mkt_msg *mmsg, *tmp;

    if (unlikely(!sub))
        return;

    if (sub->zmq_sub)
        zmq_close(sub->zmq_sub);

    list_for_each_entry_safe(mmsg, tmp, &sub->queue, struct mkt_msg, q_node) {
        sub->nr_msg--;
        list_del(&mmsg->q_node);
        free(mmsg);
    }

    free(sub);
}
