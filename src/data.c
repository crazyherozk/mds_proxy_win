#include "data.h"

int parse_topic(const char *topic, uint32_t *exchId, uint32_t *instrId)
{
    char *p1, *p2;
    int n, l, v = 0;
    char tmp[] = "000000.xx";
    
    if (strlen(topic) != sizeof(tmp) - 1)
        return -EINVAL;

    snprintf(tmp, sizeof(tmp), "%s", topic);

    p1 = tmp; 
    /*解析产品编号*/
    p2 = strchr(p1, '.');
    if (!p2)
        return -EINVAL;

    if (p1 == p2)
        return -EINVAL;

    p2[0] = '\0';
    while (p1[0] == '0') {
        p1++;
    }

    if (p1 != p2) {
        n = sscanf_s(p1, "%u%n", &v, &l);
        if (n != 1 || l != (int)(p2 - p1) || v < 0) {
            p2[0] = '.';
            return -EINVAL;
        }
    }

    p2[0] = '.';
    *instrId = v;

    /*解析市场编码*/
    p1 = &p2[1];
    if (p1[2] != '\0' || p1[0] != 'S')
        return -EINVAL;
    if (p1[1] == 'H') {
        *exchId = MDSPXY_EXCH_SSE;
    } else if (p1[1] == 'Z') {
        *exchId = MDSPXY_EXCH_SZSE;
    } else {
        return -EINVAL;
    }

    return 0;
}
