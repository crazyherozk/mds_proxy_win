#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mdspxy_api/mdspxy.h>

int main(int argc, char **argv)
{
    int rc;

    if (argc < 3) {
out:
        fprintf(stderr, "./read_file <tcp://ip:port> <topic> [topic ...]\n");
        exit(EXIT_FAILURE);
    }

    struct mds_sub *sub = mds_sub_open(argv[1]);
    if (!sub) {
        fprintf(stderr, "cannot connect publist server : %s\n", argv[1]);
        goto out;
    }


    for (int i = 2; i < argc; i++) {
        rc = mds_sub_req(sub, argv[i]);
        if (rc != 0) {
            fprintf(stderr, "subscribe [%s] failed : %s\n", argv[i],
                strerror(-rc));
            exit(EXIT_FAILURE);
        }
    }
    
    struct mds_data data;

    do {
        uint64_t seq;
        uint32_t exchId, instrId, tradeDate;
        rc = mds_sub_recv(sub, &data, 5000);
        if (rc < 0) {
            if (rc == -ETIMEDOUT)
                continue;
            break;
        }

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
    } while (1);

    if (rc < 0) {
        fprintf(stderr, "read file : %s\n", strerror(-rc));
    }

    mds_sub_close(sub);

    return EXIT_SUCCESS;
}
