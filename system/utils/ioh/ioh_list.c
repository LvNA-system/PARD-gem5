#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ioh.h"

extern struct cpdev_t *cpdev;

typedef char column_string_t [21];

column_string_t data[32][7] = {
    {  "",       "", "    PCI", "   ", "      PARD", "Support", "    "},
    {"ID", "Device", "Address", "IRQ", "Compatible", "  Guest", "DSid"},
};
int currow = 2;

void add_device(struct DEVICE *device)
{
    struct IOHubDevice *info = &device->info;
    int bus, dev, fun;

    bus = (info->pciid>>8) & 0xFF;
    dev = (info->pciid>>4) & 0xF;
    fun = (info->pciid   ) & 0xF;

    snprintf(data[currow][0], 21, "%d", info->id);
    strncpy (data[currow][1], info->ident, 21);
    snprintf(data[currow][2], 21, "%02X:%02X.%X", bus, dev, fun);
    snprintf(data[currow][3], 21, "%d", info->interrupt);
    strncpy (data[currow][4],  info->flags ? "yes" : "none", 21);
    strncpy (data[currow][5], "N/A", 21);
    if (device->guests_nr == 0)
        strncpy (data[currow][6],   "N/A", 21);
    else {
        int off = 0;
        for (int i=0; i<device->guests_nr; i++)
            off += snprintf(data[currow][6]+off, 21-off,
                            i==0 ? "%d" : ",%d", device->guests[i]);
    }
    currow++;
}

void print_table(int max_row, int max_col, column_string_t data[][7],
                 int header_cnt, const char **header_fmt, const char **body_fmt)
{
    int *width = (int *)malloc(sizeof(int)*max_col);
    int total_width = 0;

    // calc max width of each column
    for (int col=0; col<max_col; col++) {
        width[col] = 0;
        for (int row=0; row<max_row; row++) {
            if (strlen(data[row][col]) > width[col])
                width[col] = strlen(data[row][col]);
        }
        total_width += width[col] + 2;
    }

    // print table
    for (int row=0; row<max_row; row++) {
        const char ** fmt = (row >= header_cnt) ? body_fmt : header_fmt;
        if (row == header_cnt) {
            for (int i=0; i<total_width; i++) printf("-");
            printf("\n");
        }
        for (int col=0; col<max_col; col++) {
            printf(fmt ? fmt[col] : "%*s", width[col], data[row][col]);
            printf("  ");
        }
        printf("\n");
    }
}

int ioh_list(struct DEVICE *devices)
{
    struct DEVICE *pDev = devices;

    pDev = devices;
    while(pDev != NULL) {
        add_device(pDev);
        pDev = pDev->next;
    }

    printf("Device List:\n");
    print_table(currow, 7, data, 2, NULL, NULL);
    printf("\n");

    return 0;
}

uint32_t pciids_to_devmask(struct DEVICE *devices, uint16_t *pciids, int pciids_nr)
{
    uint32_t devmask = 0;

    for (int i=0; i<pciids_nr; i++) {
        struct DEVICE *pDev;
        for (pDev = devices; pDev != NULL; pDev = pDev->next) {
            if (pciids[i] == pDev->info.pciid) {
                devmask |= 1<<pDev->info.id;
                break;
            }
        }
        if (!pDev) {
            int bus = (pciids[i]>>8) & 0xFF;
            int dev = (pciids[i]>>4) & 0xF;
            int fun = (pciids[i]   ) & 0xF;
            fprintf(stderr, "error: unknown device %02d:%02d.%d\n", bus, dev, fun);
        }
    }

    return devmask;
}

int
ioh_assign(
    struct DEVICE *devices,
    uint16_t DSid, uint16_t *pciids, int pciids_nr)
{
    struct ParamEntry param;
    int row = 0;
    int free_row = -1;
    int select_row = -1;
    uint32_t assign_mask = 0;

    assign_mask = pciids_to_devmask(devices, pciids, pciids_nr);
    if (!assign_mask) {
        fprintf(stderr, "error: no valid pcidevice assigned.\n");
        return -ENODEV;
    }

    while (1) {
        // read param entry data until EOF detected
        cpdev->ops->cfgtbl_param_read_qword(cpdev, row, 0, (uint64_t *)&param);
        if (*((uint64_t *)&param) == (uint64_t)0xFFFFFFFFFFFFFFFF)
            break;

        // VALID entry
        if ((param.flags & FLAG_VALID) && (param.DSid == DSid)) {
            select_row = row;
            break;
        }

        // select a free row
        if (!(param.flags & FLAG_VALID) && (free_row==-1))
            free_row = row;

        row++;
    }

    // if it is a new DSid, register a new param entry
    if (select_row == -1) {
        if (free_row == -1)
            return -ENOMEM;
        param.flags = FLAG_VALID;
        param.DSid = DSid;
        param.device_mask = 0;
        select_row = free_row;
    }

    // update select row's device mask
    printf("DSid: %d  row: %d   0x%08X + 0x%08X ==> 0x%08X\n",
           DSid, select_row,
           param.device_mask, assign_mask, param.device_mask|assign_mask);
    param.device_mask |= assign_mask;
    return cpdev->ops->cfgtbl_param_write_qword(cpdev, select_row, 0, *(uint64_t*)&param);
}

int
ioh_release(
    struct DEVICE *devices,
    uint16_t DSid, uint16_t *pciids, int pciids_nr)
{
    struct ParamEntry param;
    int row = 0;
    int select_row = -1;
    uint32_t release_mask = 0;

    // get release mask form pciids
    release_mask = pciids_to_devmask(devices, pciids, pciids_nr);
    if (release_mask)
        release_mask = ~release_mask;

    while (1) {
        // read param entry data until EOF detected
        cpdev->ops->cfgtbl_param_read_qword(cpdev, row, 0, (uint64_t *)&param);
        if (*((uint64_t *)&param) == (uint64_t)0xFFFFFFFFFFFFFFFF)
            break;
        // VALID entry
        if ((param.flags & FLAG_VALID) && (param.DSid == DSid)) {
            select_row = row;
            break;
        }
        row++;
    }

    // if DSid not found, just return
    if (select_row == -1) {
        fprintf(stderr, "warn: DSid#%d not found.\n", DSid);
        return -ENODEV;
    }

    // update select row's device mask
    printf("DSid: %d  row: %d   0x%08X & 0x%08X ==> 0x%08X\n",
           DSid, select_row,
           param.device_mask, release_mask, param.device_mask&release_mask);
    param.device_mask &= release_mask;
    return cpdev->ops->cfgtbl_param_write_qword(cpdev, select_row, 0, *(uint64_t*)&param);
}
