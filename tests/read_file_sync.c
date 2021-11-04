#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mdspxy_api/mdspxy.h>

/**
 *实现回调即可
 *@return 0，则继续；否则中断读，但大于0可以重入
 */
static int mds_data_cb(struct mds_data *data, void *user)
{
    switch (data->type) {
        case MDSPXY_MSGTYPE_L2_ORDER:
            printf("%s,   l2 order : %d,%06d %09d\n", (const char*)user,
                   data->l2_order.exchId, data->l2_order.instrId,
                   data->l2_order.TransactTime);
            break;
        case MDSPXY_MSGTYPE_L2_TRADE:
            printf("%s,   l2 trade : %d,%06d  %09d\n", (const char*)user,
                   data->l2_trade.exchId, data->l2_trade.instrId,
                   data->l2_trade.TransactTime);
            break;
        case MDSPXY_MSGTYPE_L2_MARKET_DATA_SNAPSHOT:
            printf("%s, l2 snapshot : %d,%06d  %09d\n", (const char*)user,
                   data->l2_snapshot.exchId, data->l2_snapshot.instrId,
                   data->l2_snapshot.updateTime);
            break;
        case MDSPXY_MSGTYPE_LOSTED_CHLSEQ:
            printf("%s, losted chlseq : %d, %d ~ %d\n", (const char*)user,
                   data->chlseq.ChannelNo, data->chlseq.StartApplSeqNum,
                   data->chlseq.EndApplSeqNum);
            break;
        default:
            fprintf(stderr, "%s, unknow type : %d\n", (const char*)user,
                   data->type);
            return -1;
    }

    return 0;
}

static int mds_data_read(const char *name,
        int (*data_cb)(struct mds_data *, void *), int timeout, void *);

int main(int argc, char **argv)
{
    int rc;

    if (argc != 3) {
        fprintf(stderr, "./read_file <file> <timeout>\n");
        exit(EXIT_FAILURE);
    }

    /*返回大于0可以继续调用*/
    rc = mds_data_read(argv[1], mds_data_cb, atoi(argv[2]), "sync");

    if (rc == 0) {
        fprintf(stderr, ">>> EOF ...\n");
    } else if (rc < 0) {
        fprintf(stderr, "read file : %s\n", strerror(-rc));
    }

    return EXIT_SUCCESS;
}

static int mds_data_read(const char *name,
        int (*data_cb)(struct mds_data *, void *), int timeout, void *user)
{
    int rc;
    struct mds_data data;
    struct mds_file *file = mds_file_open(name);
    if (!file) {
        fprintf(stderr, "cannot open file : %s\n", name);
        return -errno;
    }

    mds_file_lseek(file);
    while ((rc = mds_file_read(file, &data, timeout)) > 0) {
        rc = data_cb(&data, user);
        if (rc) {
            break;
        }
    }

    mds_file_close(file);

    return rc;
}
