#ifndef __IOH_H__
#define __IOH_H__

#include <stdint.h>
#include "cpa_ioctl.h"

enum IOH_COMMAND {
    IOH_HELP = 0, IOH_LIST, IOH_ASSIGN, IOH_RELEASE, IOH_NONE
};

/**
 * SystemInfo Table
 */
struct IOHubDevice {
    uint16_t next_ptr;
    uint8_t id;
    uint8_t flags;
    uint16_t pciid;
    uint8_t interrupt;
    uint8_t reserved;
    char ident[32];
};

struct IOHubInfo
{
    uint16_t device_ptr;
    uint8_t device_nr;
    uint8_t  __res[5];
};

/**
 * Config Table
 */
struct ParamEntry {
    uint16_t flags;
    uint16_t DSid;
    uint32_t device_mask;
};
#define FLAG_VALID      0x0001

/**
 * In-memory devices structure
 */
struct DEVICE {
    struct IOHubDevice info;
    int guests_nr;
    uint16_t guests[128];
    struct DEVICE *next;
};


struct cpdev_t;
struct cpdev_ops_t {
    int (*read)(struct cpdev_t *cpdev, char *buf, int len);
    int (*write)(struct cpdev_t *cpdev, char *buf, int len);
    int (*cfgtbl_param_read_qword) 
            (struct cpdev_t *, int rowid, int offset, uint64_t *data);
    int (*cfgtbl_param_write_qword)
            (struct cpdev_t *, int rowid, int offset, uint64_t  data);
    int (*cfgtbl_stat_read_qword)
            (struct cpdev_t *, int rowid, int offset, uint64_t *data);
    int (*cfgtbl_stat_write_qword)
            (struct cpdev_t *, int rowid, int offset, uint64_t  data);
    int (*sysinfo_read)
            (struct cpdev_t *, int base, char *buf, int size);
};

struct cpdev_t {
    int fd;
    struct cpdev_ops_t *ops;
};

int open_iohcp_dev(struct cpdev_t *cpdev, const char *name);
void close_iohcp_dev(struct cpdev_t *cpdev);


// IOH command
int ioh_list(struct DEVICE *devices);
int ioh_assign(struct DEVICE *devices, uint16_t DSid, uint16_t *pciids, int pciids_nr);
int ioh_release(struct DEVICE *devices, uint16_t DSid, uint16_t *pciids, int pciids_nr);

#endif	// __IOH_H__
