#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ich.h"

extern struct cpdev_t *cpdev;

int ich_list()
{
    printf("!!UMIMPL!! Device List:\n");
    printf("\n");
    return 0;
}

int
ich_assign(uint16_t DSid)
{
    struct ParamEntry param;
    int row = 0;
    int free_row = -1;

    while (1) {
        // read param entry data until EOF detected
        cpdev->ops->cfgtbl_param_read_qword(cpdev, DSid, row, 0, (uint64_t *)&param);
        if (*((uint64_t *)&param) == (uint64_t)0xFFFFFFFFFFFFFFFF)
            break;

        // VALID entry
        if ((param.flags & FLAG_VALID) && (param.DSid == DSid)) {
            printf("DSid#%d already register in ICH.\n", DSid);
            return 0;
        }

        // select a free row
        if (!(param.flags & FLAG_VALID) && (free_row==-1)) {
            free_row = row;
            break;
        }

        row++;
    }

    // if it is a new DSid, register a new param entry
    if (free_row == -1)
        return -ENOMEM;
    memset(&param, -1, sizeof(param));
    param.flags = FLAG_VALID;
    param.DSid = DSid;
    param.selected = free_row;
    param.regbase[0] = 0x80000000000003f8;
    param.regbase[1] = 0x8000000000000070;
    param.regbase[2] = 0x8000000000000040;
    param.regbase[3] = 0xFEC00000;
    param.intlines[free_row] = 2;
    param.intlines[free_row+4] = 4;
    param.intlines[8] = 8;

    // update select row's device mask
    printf("ICH: register DSid#%d @ row%d\n", DSid, free_row);

    uint64_t *ptr = (uint64_t *)&param;
    for (int i=1; i<sizeof(param)/sizeof(uint64_t); i++) {
        int ret = cpdev->ops->cfgtbl_param_write_qword(cpdev, DSid, free_row,
                                                       i*sizeof(uint64_t), *(ptr+i));
        if (ret < 0)
            return ret;
    }
    return cpdev->ops->cfgtbl_param_write_qword(cpdev, DSid, free_row, 0, *ptr);
}

int
ich_release(uint16_t DSid)
{
    printf("!!UNIMPL!! ich_release\n");
    return 0;
}

int
ich_irqmap(uint16_t DSid, int from, int to)
{
    struct ParamEntry param;
    int row = 0;
    int selected_row = -1;

    while (1) {
        // read param entry data until EOF detected
        cpdev->ops->cfgtbl_param_read_qword(cpdev, DSid, row, 0, (uint64_t *)&param);
        if (*((uint64_t *)&param) == (uint64_t)0xFFFFFFFFFFFFFFFF)
            break;

        // VALID entry
        if ((param.flags & FLAG_VALID) && (param.DSid == DSid)) {
            selected_row = row;
            break;
        }

        row++;
    }

    // if DSid not exist, return ENODEV
    if (selected_row == -1)
        return -ENODEV;

    int offset = ((char *)&param.intlines[from] - (char *)&param)/sizeof(uint64_t);
    cpdev->ops->cfgtbl_param_read_qword(cpdev, DSid, row, offset*sizeof(uint64_t),
                                        (uint64_t *)&param + offset);
    param.intlines[from] = to;

    // update select row's device mask
    printf("ICH: IRQ remap DSid#%d @ row#%d : %d => %d\n",
            DSid, selected_row, from, to);

    return cpdev->ops->cfgtbl_param_write_qword(cpdev, DSid, selected_row, offset*sizeof(uint64_t),
                                                *((uint64_t *)&param + offset));
}
