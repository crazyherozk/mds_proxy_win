#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mdspxy_api/mdspxy.h>

int main(int argc, char **argv)
{
    if (argc != 3) {
out:
        fprintf(stderr, "./read_file <file> <timeout>\n");
        exit(EXIT_FAILURE);
    }

    struct mds_file *file = mds_file_open(argv[1]);
    if (!file) {
        fprintf(stderr, "cannot open file : %s\n", argv[1]);
        goto out;
    }

    int rc;
    int to = atoi(argv[2]);
    struct mds_data data;

    while ((rc = mds_file_read(file, &data, to)) > 0) {
        uint64_t seq;
        uint32_t exchId, instrId, tradeDate;
        get_common_info(&data, &exchId, &instrId, &tradeDate, &seq);
        switch (data.type) {
            case MDSPXY_MSGTYPE_L2_ORDER:
                printf("   l2 order : %010llu - %02d,%06d\n",
                       seq, exchId, instrId);
                break;
            case MDSPXY_MSGTYPE_L2_TRADE:
                printf("   l2 trade : %010llu - %02d,%06d\n",
                       seq, exchId, instrId);
                break;
            case MDSPXY_MSGTYPE_L2_MARKET_DATA_SNAPSHOT:
                printf("l2 snapshot : %010llu - %02d,%06d\n",
                       seq, exchId, instrId);
                break;
            case MDSPXY_MSGTYPE_LOSTED_CHLSEQ:
                printf("losted chlseq : %010llu - %02d, %d ~ %d\n",
                       seq, data.chlseq.ChannelNo, data.chlseq.StartApplSeqNum,
                       data.chlseq.EndApplSeqNum);
                break;
            default:
                fprintf(stderr, "unknow type : %d\n", data.type);
                abort();
        }
    }

    if (rc == 0) {
        fprintf(stderr, ">>> EOF ...\n");
    } else if (rc < 0) {
        fprintf(stderr, "read file : %s\n", strerror(-rc));
    }

    mds_file_close(file);

    return EXIT_SUCCESS;
}
