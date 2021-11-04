#include "encode.h"

#define encode__decode(dst, opt, var)       \
do {                                        \
    switch(sizeof(var)) {                   \
    case 1: (dst) = (var); break;           \
    case 2: (dst) = opt##s(var);break;      \
    case 4: (dst) = opt##l(var);break;      \
    case 8: (dst) = opt##ll(var);break;     \
    default:abort();                        \
    }                                       \
} while (0)

#define encode__field(src, dst, field)      \
do { encode__decode((dst)->field, hton, (src)->field); } while(0)

#define decode__field(src, dst, field)      \
do { encode__decode((dst)->field, ntoh, (src)->field); } while(0)

void encode_chlseq(const struct losted_chlseq *src, struct losted_chlseq *dst)
{
    struct losted_chlseq *__s = (struct losted_chlseq*)src;
    memset(dst->__pad0, 0, sizeof(dst->__pad0));
    encode__field(__s, dst, seq);
    encode__field(__s, dst, exchId);
    encode__field(__s, dst, instrId);
    encode__field(__s, dst, tradeDate);
    encode__field(__s, dst, ChannelNo);
    encode__field(__s, dst, StartApplSeqNum);
    encode__field(__s, dst, EndApplSeqNum);
}

void encode_l2order(const struct local_l2order *src, struct local_l2order *dst)
{
    struct local_l2order *__s = (struct local_l2order*)src;
    encode__field(__s, dst, exchId);
    encode__field(__s, dst, mdProductType);
    encode__field(__s, dst, Side);
    encode__field(__s, dst, OrderType);
    encode__field(__s, dst, tradeDate);
    encode__field(__s, dst, TransactTime);
    encode__field(__s, dst, instrId);
    encode__field(__s, dst, ChannelNo);
    encode__field(__s, dst, ApplSeqNum);
    encode__field(__s, dst, Price);
    encode__field(__s, dst, OrderQty);
    encode__field(__s, dst, SseBizIndex);
    encode__field(__s, dst, SseOrderNo);
}

void encode_l2trade(const struct local_l2trade *src, struct local_l2trade *dst)
{
    struct local_l2trade *__s = (struct local_l2trade *)src;
    encode__field(__s, dst, exchId);
    encode__field(__s, dst, mdProductType);
    encode__field(__s, dst, ExecType);
    encode__field(__s, dst, TradeBSFlag);
    encode__field(__s, dst, tradeDate);
    encode__field(__s, dst, TransactTime);
    encode__field(__s, dst, instrId);
    encode__field(__s, dst, ChannelNo);
    encode__field(__s, dst, ApplSeqNum);
    encode__field(__s, dst, TradePrice);
    encode__field(__s, dst, TradeQty);
    encode__field(__s, dst, TradeMoney);
    encode__field(__s, dst, BidApplSeqNum);
    encode__field(__s, dst, OfferApplSeqNum);
    encode__field(__s, dst, SseBizIndex);
}

void encode_l2snapshot(const struct local_l2snapshot *src,
        struct local_l2snapshot *dst)
{
    struct local_l2snapshot *__s = (struct local_l2snapshot*)src;
    dst->__pad0 = 0;
    encode__field(__s, dst, exchId);
    encode__field(__s, dst, mdProductType);
    encode__field(__s, dst, tradeDate);
    encode__field(__s, dst, updateTime);
    encode__field(__s, dst, instrId);
    encode__field(__s, dst, NumTrades);
    encode__field(__s, dst, TotalVolumeTraded);
    encode__field(__s, dst, TotalValueTraded);
    encode__field(__s, dst, TotalBidQty);
    encode__field(__s, dst, TotalOfferQty);

    encode__field(__s, dst, PrevClosePx);
    encode__field(__s, dst, OpenPx);
    encode__field(__s, dst, HighPx);
    encode__field(__s, dst, LowPx);
    encode__field(__s, dst, TradePx);
    encode__field(__s, dst, ClosePx);
    encode__field(__s, dst, IOPV);
    encode__field(__s, dst, NAV);
    encode__field(__s, dst, TotalLongPosition);
    encode__field(__s, dst, WeightedAvgBidPx);
    encode__field(__s, dst, WeightedAvgOfferPx);
    encode__field(__s, dst, BidPriceLevel);
    encode__field(__s, dst, OfferPriceLevel);

    BUILD_BUG_ON(ARRAY_SIZE(src->BidLevels) != ARRAY_SIZE(src->OfferLevels));
    for (int i = 0; i < ARRAY_SIZE(src->BidLevels); i++) {
        encode__field(&__s->BidLevels[i], &dst->BidLevels[i], Price);
        encode__field(&__s->BidLevels[i], &dst->BidLevels[i], OrderQty);
        encode__field(&__s->BidLevels[i], &dst->BidLevels[i], NumberOfOrders);

        encode__field(&__s->OfferLevels[i], &dst->OfferLevels[i], Price);
        encode__field(&__s->OfferLevels[i], &dst->OfferLevels[i], OrderQty);
        encode__field(&__s->OfferLevels[i],&dst->OfferLevels[i],NumberOfOrders);
    }
}

void decode_chlseq(const struct losted_chlseq *src, struct losted_chlseq *dst)
{
    struct losted_chlseq *__s = (struct losted_chlseq*)src;
    decode__field(__s, dst, seq);
    decode__field(__s, dst, exchId);
    decode__field(__s, dst, instrId);
    decode__field(__s, dst, tradeDate);
    decode__field(__s, dst, ChannelNo);
    decode__field(__s, dst, StartApplSeqNum);
    decode__field(__s, dst, EndApplSeqNum);
}

void decode_l2order(const struct local_l2order *src, struct local_l2order *dst)

{
    struct local_l2order *__s = (struct local_l2order*)src;
    decode__field(__s, dst, exchId);
    decode__field(__s, dst, mdProductType);
    decode__field(__s, dst, Side);
    decode__field(__s, dst, OrderType);
    decode__field(__s, dst, tradeDate);
    decode__field(__s, dst, TransactTime);
    decode__field(__s, dst, instrId);
    decode__field(__s, dst, ChannelNo);
    decode__field(__s, dst, ApplSeqNum);
    decode__field(__s, dst, Price);
    decode__field(__s, dst, OrderQty);
    decode__field(__s, dst, SseBizIndex);
    decode__field(__s, dst, SseOrderNo);
}

void decode_l2trade(const struct local_l2trade *src, struct local_l2trade *dst)
{
    struct local_l2trade *__s = (struct local_l2trade *)src;
    decode__field(__s, dst, exchId);
    decode__field(__s, dst, mdProductType);
    decode__field(__s, dst, ExecType);
    decode__field(__s, dst, TradeBSFlag);
    decode__field(__s, dst, tradeDate);
    decode__field(__s, dst, TransactTime);
    decode__field(__s, dst, instrId);
    decode__field(__s, dst, ChannelNo);
    decode__field(__s, dst, ApplSeqNum);
    decode__field(__s, dst, TradePrice);
    decode__field(__s, dst, TradeQty);
    decode__field(__s, dst, TradeMoney);
    decode__field(__s, dst, BidApplSeqNum);
    decode__field(__s, dst, OfferApplSeqNum);
    decode__field(__s, dst, SseBizIndex);
}

void decode_l2snapshot(const struct local_l2snapshot *src,
                       struct local_l2snapshot *dst)
{
    struct local_l2snapshot *__s = (struct local_l2snapshot*)src;
    dst->__pad0 = 0;
    decode__field(__s, dst, exchId);
    decode__field(__s, dst, mdProductType);
    decode__field(__s, dst, tradeDate);
    decode__field(__s, dst, updateTime);
    decode__field(__s, dst, instrId);
    decode__field(__s, dst, NumTrades);
    decode__field(__s, dst, TotalVolumeTraded);
    decode__field(__s, dst, TotalValueTraded);
    decode__field(__s, dst, TotalBidQty);
    decode__field(__s, dst, TotalOfferQty);

    decode__field(__s, dst, PrevClosePx);
    decode__field(__s, dst, OpenPx);
    decode__field(__s, dst, HighPx);
    decode__field(__s, dst, LowPx);
    decode__field(__s, dst, TradePx);
    decode__field(__s, dst, ClosePx);
    decode__field(__s, dst, IOPV);
    decode__field(__s, dst, NAV);
    decode__field(__s, dst, TotalLongPosition);
    decode__field(__s, dst, WeightedAvgBidPx);
    decode__field(__s, dst, WeightedAvgOfferPx);
    decode__field(__s, dst, BidPriceLevel);
    decode__field(__s, dst, OfferPriceLevel);

    BUILD_BUG_ON(ARRAY_SIZE(src->BidLevels) != ARRAY_SIZE(src->OfferLevels));
    for (int i = 0; i < ARRAY_SIZE(src->BidLevels); i++) {
        decode__field(&__s->BidLevels[i], &dst->BidLevels[i], Price);
        decode__field(&__s->BidLevels[i], &dst->BidLevels[i], OrderQty);
        decode__field(&__s->BidLevels[i], &dst->BidLevels[i], NumberOfOrders);

        decode__field(&__s->OfferLevels[i], &dst->OfferLevels[i], Price);
        decode__field(&__s->OfferLevels[i], &dst->OfferLevels[i], OrderQty);
        decode__field(&__s->OfferLevels[i],&dst->OfferLevels[i],NumberOfOrders);
    }
}

void free__mmsg_list(struct list_head *head)
{
    struct mkt_msg *msg, *next;

    list_for_each_entry_safe(msg, next, head, struct mkt_msg, q_node) {
        list_del(&msg->q_node);
        free(msg);
    }

    return;
}

static inline void mmsg_encode(struct mkt_msg *msg)
{
    switch (msg->msgId) {
        case MDSPXY_MSGTYPE_L2_ORDER:
            encode_l2order(&msg->order, &msg->order);
            break;
        case MDSPXY_MSGTYPE_L2_TRADE:
            encode_l2trade(&msg->trade, &msg->trade);
            break;
        case MDSPXY_MSGTYPE_L2_MARKET_DATA_SNAPSHOT:
            encode_l2snapshot(&msg->snapshot, &msg->snapshot);
            break;
        case MDSPXY_MSGTYPE_LOSTED_CHLSEQ:
            encode_chlseq(&msg->chlseq, &msg->chlseq);
            break;
        default:
            abort();
    }
}

static inline void mmsg_decode(struct mkt_msg *msg)
{
    switch (msg->msgId) {
        case MDSPXY_MSGTYPE_L2_ORDER:
            decode_l2order(&msg->order, &msg->order);
            break;
        case MDSPXY_MSGTYPE_L2_TRADE:
            decode_l2trade(&msg->trade, &msg->trade);
            break;
        case MDSPXY_MSGTYPE_L2_MARKET_DATA_SNAPSHOT:
            decode_l2snapshot(&msg->snapshot, &msg->snapshot);
            break;
        case MDSPXY_MSGTYPE_LOSTED_CHLSEQ:
            decode_chlseq(&msg->chlseq, &msg->chlseq);
            break;
        default:
            abort();
    }
}

ssize_t body_size(struct list_head *head)
{
    ssize_t l, s = 0;
    struct mkt_msg *msg;

    list_for_each_entry(msg, head, struct mkt_msg, q_node) {
        l = mmsg_size(msg);
        assert(l > 0);
        s += l;
    }

    return s;
}

void encode_head(struct list_head *head, uint32_t sum, struct batch_head *bh)
{
    uint32_t nr = 0;
    struct mkt_msg *msg;
    struct segment_head *sh = &bh->seg_head[0];

    list_for_each_entry(msg, head, struct mkt_msg, q_node) {
        ssize_t l = mmsg_size(msg);
        assert(l > 0);
        sh[nr].length = htonl((uint32_t)l);
        sh[nr].type = htonl((uint32_t)msg->msgId);
        nr++;
    }

    bh->hdr_checksum = 0;
    bh->__pad = 0xFFFFFFFF;
    bh->nr_payload = htonl(nr);
    bh->body_checksum = htonl(sum);

    sum = byteCrc32(bh, head_size(nr), 0);
    bh->hdr_checksum = htonl(sum);

    return;
}

size_t encode_body(struct list_head *head, uint32_t *sum, struct _iovec *iovec)
{
    size_t l = 0, ll = 0;
    struct mkt_msg *msg;

    if (sum)
        *sum = 0;

    list_for_each_entry(msg, head, struct mkt_msg, q_node) {
        mmsg_encode(msg);
        l = mmsg_size(msg);
        if (sum)
            *sum = byteCrc32(msg->data, l, *sum);
        ll += l;
        if (iovec) {
            iovec->iov_len = l;
            iovec->iov_base = msg->data;
            iovec++;
        }
    }

    return ll;
}

void decode_body(struct list_head *head, uint32_t *sum)
{
    size_t l = 0;
    struct mkt_msg *msg;

    if (sum)
        *sum = 0;

    list_for_each_entry(msg, head, struct mkt_msg, q_node) {
        l = mmsg_size(msg);
        assert(l>0);
        if (sum)
            *sum = byteCrc32(msg->data, l, *sum);
        mmsg_decode(msg);
    }

    return;
}
