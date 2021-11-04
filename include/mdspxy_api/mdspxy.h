#ifndef __mdspxy_h__
#define __mdspxy_h__

#include <stdint.h>

#if defined(BUILD_MDSPXY_DLL)
# define MDSPXY_EXPORT_SYMBOL __declspec(dllexport)
#elif defined(LINK_MDSPXY_DLL)
# define MDSPXY_EXPORT_SYMBOL __declspec(dllimport)
#else
# define MDSPXY_EXPORT_SYMBOL
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * 交易所代码
 * eMdsExchangeIdT
 */
enum {
    /** 未定义的交易所代码 */
    MDSPXY_EXCH_UNDEFINE                   = 0,
    /** 交易所-上交所 */
    MDSPXY_EXCH_SSE                        = 1,
    /** 交易所-深交所 */
    MDSPXY_EXCH_SZSE                       = 2,

    /** 交易所-最大值的位移数 */
    _MDSPXY_EXCH_MAX_SHIFT                 = 2,

    MDSPXY_EXCH_MAX                        = 1U << _MDSPXY_EXCH_MAX_SHIFT,
};

/**
 * 消息类型
 * eMdsMsgTypeT
 */
enum {
    /** Level2 市场行情快照 (20/0x14) */
    MDSPXY_MSGTYPE_L2_MARKET_DATA_SNAPSHOT = 20,
    /** Level2 逐笔成交行情 (22/0x16) */
    MDSPXY_MSGTYPE_L2_TRADE                = 22,
    /** Level2 深交所逐笔委托行情 (23/0x17, 仅适用于深交所) */
    MDSPXY_MSGTYPE_L2_ORDER                = 23,

    /** 自定义的数据类型*/
    MDSPXY_MSGTYPE_LOSTED_CHLSEQ           = 0x8001U,
};

/**
 * 行情类别 (和交易端的产品类型不同, 行情数据中的产品类型只是用于区分是现货行情还是衍生品行情)
 * eMdsMdProductTypeT
 */
enum {
    MDSPXY_MD_PRODUCT_TYPE_STOCK           = 1, /**< 股票(基金/债券) */
    MDSPXY_MD_PRODUCT_TYPE_INDEX           = 2, /**< 指数 */
    MDSPXY_MD_PRODUCT_TYPE_OPTION          = 3, /**< 期权 */
};

/**
 * Level2逐笔成交的内外盘标志
 * - 仅适用于上交所 ('B'=外盘,主动买, 'S'=内盘,主动卖, 'N'=未知)
 * - 对于深交所, 将固定取值为 'N'(未知)
 * eMdsL2TradeBSFlagT
 */
enum {
    MDSPXY_L2_TRADE_BSFLAG_BUY             = 'B', /**< L2内外盘标志 - 外盘,主动买 */
    MDSPXY_L2_TRADE_BSFLAG_SELL            = 'S', /**< L2内外盘标志 - 内盘,主动卖 */
    MDSPXY_L2_TRADE_BSFLAG_UNKNOWN         = 'N'  /**< L2内外盘标志 - 未知 */
};

/**
 * Level2逐笔成交的成交类别
 * - 仅适用于深交所 ('4'=撤销, 'F'=成交)
 * - 对于上交所, 将固定取值为 'F'(成交)
 * eMdsL2TradeExecTypeT
 */
enum {
    MDSPXY_L2_TRADE_EXECTYPE_CANCELED      = '4', /**< L2执行类型 - 已撤销 */
    MDSPXY_L2_TRADE_EXECTYPE_TRADE         = 'F'  /**< L2执行类型 - 已成交 */
};

/*以下结构体注意对齐*/

struct local_pricelvl_entry {
    int32_t Price;/**< 委托价格 */
    int32_t NumberOfOrders;/**< 价位总委托笔数 (Level1不揭示该值, 固定为0) */
    int64_t OrderQty;/**< 委托数量 */
};

struct local_l2order {
    uint64_t seq;  /**< 本地序号，用于去重，从1开始，并且每个交易日更新*/
    uint8_t exchId; /**< 交易所代码(沪/深) @see eMdsExchangeIdT */
    uint8_t mdProductType; /**< 行情类别 (股票/指数/期权) @see eMdsMdProductTypeT */
    /**
     * 买卖方向
     * - 深交所: '1'=买, '2'=卖, 'G'=借入, 'F'=出借
     * - 上交所: '1'=买, '2'=卖
     */
    char    Side;
    /**
     * 订单类型
     * - 深交所: '1'=市价, '2'=限价, 'U'=本方最优
     * - 上交所: 'A'=委托订单-增加(新订单), 'D'=委托订单-删除(撤单)
     */
    char    OrderType;

    int32_t tradeDate; /**< 交易日期 YYYYMMDD (自然日) */
    int32_t TransactTime;/**< 委托时间 HHMMSSsss */

    int32_t instrId; /**< 产品代码 (转换为整数类型的产品代码) */
    int32_t ChannelNo; /**< 频道代码 [0..9999] */
    int32_t ApplSeqNum; /**< 委托序号 (从1开始, 按频道连续) */

    int32_t Price; /**< 委托价格 (价格单位精确到元后四位, 即: 1元=10000) */
    /**
     * 委托数量
     * - @note 对于上交所, 该字段的含义为剩余委托量或撤单数量:
     *   - 对于上交所, 当 OrderType='A' 时, 该字段表示的是剩余委托量 (竞价撮合成交后的剩余委托数量)
     *   - 对于上交所, 当 OrderType='D' 时, 该字段表示的是撤单数量
     */
    int32_t OrderQty;

    /**
     * 业务序列号 (仅适用于上交所)
     * - 仅适用于上交所, 逐笔成交+逐笔委托统一编号, 从1开始, 按频道连续
     * - 对于深交所, 该字段将固定为 0
     */
    uint32_t SseBizIndex;
    uint32_t __pad0;

    /**
     * 原始订单号 (仅适用于上交所)
     * - 仅适用于上交所, 和逐笔成交中的买方/卖方订单号相对应
     * - 对于深交所, 该字段将固定为 0
     */
    uint64_t SseOrderNo;
};

struct local_l2trade {
    uint64_t seq; /**< 本地序号，用于去重，从1开始，并且每个交易日更新*/
    uint8_t exchId; /**< 交易所代码(沪/深) @see eMdsExchangeIdT */
    uint8_t mdProductType; /**< 行情类别 (股票/指数/期权) @see eMdsMdProductTypeT */

    /**
     * 成交类别 (仅适用于深交所, '4'=撤销, 'F'=成交)
     * 对于上证, 将固定设置为 'F'(成交)
     * @see eMdsL2TradeExecTypeT
     */
    char    ExecType;

    /**
     * 内外盘标志 (仅适用于上证, 'B'=外盘,主动买, 'S'=内盘,主动卖, 'N'=未知)
     * 对于深交所, 将固定设置为 'N'(未知)
     * @see eMdsL2TradeBSFlagT
     */
    char    TradeBSFlag;
    int32_t tradeDate;  /**< 交易日期 (YYYYMMDD, 非官方数据) */
    int32_t TransactTime;/**< 成交时间 (HHMMSSsss) */

    int32_t instrId; /**< 产品代码 (转换为整数类型的产品代码) */
    int32_t ChannelNo; /**< 成交通道/频道代码 [0..9999] */

    /**
     * 深交所消息记录号/上交所成交序号 (从1开始, 按频道连续)
     * - 深交所为逐笔成交+逐笔委托统一编号
     * - 上交所为逐笔成交独立编号 (TradeIndex)
     */
    int32_t ApplSeqNum;

    /**
     * 业务序列号 (仅适用于上交所)
     * - 仅适用于上交所, 逐笔成交+逐笔委托统一编号, 从1开始, 按频道连续
     * - 对于深交所, 该字段将固定为 0
     */
    uint32_t SseBizIndex;
    uint32_t __pad0;

    int32_t TradePrice; /**< 成交价格 (价格单位精确到元后四位, 即: 1元=10000) */
    int32_t TradeQty;   /**< 成交数量 (上海债券的数量单位为: 手) */
    int64_t TradeMoney; /**< 成交金额 (金额单位精确到元后四位, 即: 1元=10000) */

    int64_t BidApplSeqNum; /**< 买方订单号 (从 1 开始计数, 0 表示无对应委托) */
    int64_t OfferApplSeqNum; /**< 卖方订单号 (从 1 开始计数, 0 表示无对应委托) */
};

struct local_l2snapshot {
    uint64_t seq; /**< 本地序号，用于去重，从1开始，并且每个交易日更新*/
    uint8_t exchId; /**< 交易所代码(沪/深) @see eMdsExchangeIdT */
    uint8_t mdProductType; /**< 行情类别 (股票/指数/期权) @see eMdsMdProductTypeT */
    uint16_t __pad0; /**< 填充*/
    int32_t tradeDate; /**< 交易日期 (YYYYMMDD, 8位整型数值) */
    int32_t updateTime; /**< 行情时间 (HHMMSSsss, 交易所时间) */
    int32_t instrId; /**< 产品代码 (转换为整数类型的产品代码) */

    /**
     * 产品实时阶段及标志 C8 / C4
     *
     * 上交所股票 (C8):
     *  -# 第 0 位:
     *      - ‘S’表示启动 (开市前) 时段, ‘C’表示开盘集合竞价时段, ‘T’表示连续交易时段,
     *      - ‘E’表示闭市时段, ‘P’表示产品停牌,
     *      - ‘M’表示可恢复交易的熔断时段 (盘中集合竞价), ‘N’表示不可恢复交易的熔断时段 (暂停交易至闭市),
     *      - ‘U’表示收盘集合竞价时段。
     *  -# 第 1 位:
     *      - ‘0’表示此产品不可正常交易,
     *      - ‘1’表示此产品可正常交易,
     *      - 无意义填空格。
     *      - 在产品进入开盘集合竞价、连续交易、收盘集合竞价、熔断(盘中集合竞价)状态时值为‘1’,
     *        在产品进入停牌、熔断(暂停交易至闭市)状态时值为‘0’, 且闭市后保持该产品闭市前的是否可正常交易状态。
     *  -# 第 2 位:
     *      - ‘0’表示未上市, ‘1’表示已上市。
     *  -# 第 3 位:
     *      - ‘0’表示此产品在当前时段不接受订单申报,
     *      - ‘1’表示此产品在当前时段可接受订单申报。
     *      - 仅在交易时段有效，在非交易时段无效。无意义填空格。
     *
     * 上交所期权 (C4):
     *  -# 第 0 位:
     *      - ‘S’表示启动(开市前)时段, ‘C’表示集合竞价时段, ‘T’表示连续交易时段,
     *      - ‘B’表示休市时段, ‘E’表示闭市时段, ‘V’表示波动性中断, ‘P’表示临时停牌, ‘U’收盘集合竞价。
     *      - ‘M’表示可恢复交易的熔断 (盘中集合竞价), ‘N’表示不可恢复交易的熔断 (暂停交易至闭市)
     *  -# 第 1 位:
     *      - ‘0’表示未连续停牌, ‘1’表示连续停牌。(预留, 暂填空格)
     *  -# 第 2 位:
     *      - ‘0’表示不限制开仓, ‘1’表示限制备兑开仓, ‘2’表示卖出开仓, ‘3’表示限制卖出开仓、备兑开仓,
     *      - ‘4’表示限制买入开仓, ‘5’表示限制买入开仓、备兑开仓, ‘6’表示限制买入开仓、卖出开仓,
     *      - ‘7’表示限制买入开仓、卖出开仓、备兑开仓
     *  -# 第 3 位:
     *      - ‘0’表示此产品在当前时段不接受进行新订单申报,
     *      - ‘1’表示此产品在当前时段可接受进行新订单申报。
     *      - 仅在交易时段有效，在非交易时段无效。
     *
     * 深交所 (C8):
     *  -# 第 0 位:
     *      - S=启动(开市前) O=开盘集合竞价 T=连续竞价
     *      - B=休市 C=收盘集合竞价 E=已闭市 H=临时停牌
     *      - A=盘后交易 V=波动性中断
     *  -# 第 1 位:
     *      - 0=正常状态
     *      - 1=全天停牌
     */
    char    TradingPhaseCode[8];

    uint64_t NumTrades;  /**< 成交笔数 */
    uint64_t TotalVolumeTraded; /**< 成交总量 */
    int64_t TotalValueTraded; /**< 成交总金额 (金额单位精确到元后四位,即:1元=10000) */

    int64_t TotalBidQty; /**< 委托买入总量 */
    int64_t TotalOfferQty; /**< 委托卖出总量 */

    int32_t PrevClosePx; /**< 昨日收盘价 (价格单位精确到元后四位, 即:1元=10000) */
    int32_t OpenPx; /**< 今开盘价 (价格单位精确到元后四位, 即: 1元=10000) */
    int32_t HighPx; /**< 最高价 */
    int32_t LowPx; /**< 最低价 */
    int32_t TradePx; /**< 成交价 */
    int32_t ClosePx; /**< 今收盘价/期权收盘价 (仅适用于上交所, 深交所行情没有单独的收盘价) */

    int32_t IOPV; /**< 基金份额参考净值/ETF申赎的单位参考净值 (适用于基金) */
    int32_t NAV;  /**< 基金 T-1 日净值 (适用于基金) */
    uint64_t TotalLongPosition; /**< 合约总持仓量 (适用于期权) */

    int32_t WeightedAvgBidPx; /**< 加权平均委买价格 */
    int32_t WeightedAvgOfferPx; /**< 加权平均委卖价格 */
    int32_t BidPriceLevel; /**< 买方委托价位数 (实际的委托价位总数, 仅适用于上交所) */
    int32_t OfferPriceLevel; /**< 卖方委托价位数 (实际的委托价位总数, 仅适用于上交所) */
    struct local_pricelvl_entry BidLevels[10]; /**< 十档买盘价位信息 */
    struct local_pricelvl_entry OfferLevels[10]; /**< 十档卖盘价位信息 */
};

/**
 * 缺失的通道序号
 * 如果 startApplSeqNum 和 endApplSeqNum 都为 -1 则是一条心跳数据
 */
struct losted_chlseq {
    uint64_t seq;  /**< 本地序号，用于去重，从1开始，并且每个交易日更新*/
    uint8_t exchId; /**< 交易所代码(沪/深) @see eMdsExchangeIdT */
    uint8_t __pad0[3];
    int32_t instrId; /**< 产品代码 (不存在的，用来命名文件，使用者可以忽略) */
    uint32_t tradeDate; /**< 交易日期*/
    int32_t ChannelNo; /**< 成交通道/频道代码 [0..9999] */
    int32_t StartApplSeqNum; /**< 缺失的起始序号*/
    int32_t EndApplSeqNum; /**< 缺失的结束序号*/
};

struct mds_data {
    int type; /**< 数据类型 @see eMdsMsgTypeT */
    union {
        struct losted_chlseq chlseq;
        struct local_l2order l2_order;
        struct local_l2trade l2_trade;
        struct local_l2snapshot l2_snapshot;
        char data[1];
    };
};


/*
 * 文件分为2种类型，不以 tmp 为后缀的闭合文件，他们将不会再更新
 * 以及以 tmp 为后缀的非闭合文件，他们会更新
 */

struct mds_file;
MDSPXY_EXPORT_SYMBOL struct mds_file *mds_file_open(const char *);
MDSPXY_EXPORT_SYMBOL void mds_file_close(struct mds_file *);

/**
 * @param data 输出
 * @param timeout 是否等待数据，毫秒单位，如果是闭合文件，则无效；
 *                如果为0则在没有数据的情况下立即返回 -EAGAIN。
 * @return 1 读取到数据，0 文件尾，如果是 非闭合文件 读取到尾部是返回 -EAGAIN
 *         -EINVAL 无效的参数
 *         -EPROTO 遇到不一致的数据，必须关闭
 *         -ENOMEM 没有足够的内存读取数据到缓存，可以重试
 *         -EAGAIN 当前是非闭合文件且没有足够数据，可以重试
 *         -ETIMEDOUT 当前时非闭合文件且没有足够数据，等待超时，可以重试
 *         -EPERM 没有侦听文件的权限
 *         -ENOENT 文件被删除了
 */
MDSPXY_EXPORT_SYMBOL int mds_file_read(struct mds_file *, struct mds_data *data,
        int timeout);

/**
 * 丢弃前面的数据，将文件定位到最新数据
 * 如果是以闭合的文件将结束读取
 */
MDSPXY_EXPORT_SYMBOL int mds_file_lseek(struct mds_file *);

/**
 * 获取通用信息
 */
MDSPXY_EXPORT_SYMBOL void get_common_info(const struct mds_data *data,
        uint32_t *exchId, uint32_t *instrId, uint32_t *tradeDate, uint64_t *seq)
        ;

/**
 * 打开订阅
 * 订阅地址为 tcp://ip:port 形式，如 tcp://localhost:1024
 */

struct mds_sub;
MDSPXY_EXPORT_SYMBOL struct mds_sub *mds_sub_open(const char *addr);

/**
 * 订阅请求，topic 为可读字符串：股票+市场，如 000830.SZ 或 128033.SZ
 * 股票编码不足6位的前导补0字符
 * 可以调用多次，订阅多支股票的行情
 */

MDSPXY_EXPORT_SYMBOL int mds_sub_req(struct mds_sub *, const char *topic);

/**
 * 取消订阅
 */
MDSPXY_EXPORT_SYMBOL int mds_sub_cancel(struct mds_sub *, const char *topic);

/**
 * 读取订阅消息
 * @return 0
 *         -EINVAL 无效的参数
 *         -EPROTO 遇到不一致的数据，必须关闭
 *         -ENOMEM 没有足够的内存读取数据到缓存，可以重试
 *         -EAGAIN 当前是非闭合文件且没有足够数据，可以重试
 *         -ETIMEDOUT 当前时非闭合文件且没有足够数据，等待超时，可以重试
 */
MDSPXY_EXPORT_SYMBOL int mds_sub_recv(struct mds_sub *, struct mds_data *data,
        int timeout);

MDSPXY_EXPORT_SYMBOL void mds_sub_close(struct mds_sub *);

/**
 * 打开一个文件+订阅的句柄
 * 这个句柄当接收到订阅消息时可以使用当前落地的数据文件自动补全订阅数据
 * 传入的目录路径必须是包含 落地交易日目录的路径
 */
struct mds_fsub;
MDSPXY_EXPORT_SYMBOL struct mds_fsub *mds_fsub_open(const char *addr,
        const char *path);

/**
 * 订阅请求，参考 mds_sub_req
 */
MDSPXY_EXPORT_SYMBOL int mds_fsub_req(struct mds_fsub *, const char *topic);

/**
 * 取消订阅
 */
MDSPXY_EXPORT_SYMBOL int mds_fsub_cancel(struct mds_fsub *, const char *topic);

/**
 * 读取订阅消息
 * @see mds_sub_recv(); 
 */
MDSPXY_EXPORT_SYMBOL int mds_fsub_recv(struct mds_fsub *, struct mds_data *data,
        int timeout);

MDSPXY_EXPORT_SYMBOL void mds_fsub_close(struct mds_fsub *);

#if defined(__cplusplus)
}
#endif

#endif
