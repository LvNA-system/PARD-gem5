#ifndef __ICH_H__
#define __ICH_H__

#include <stdint.h>
#include "cpa_ioctl.h"

enum ICH_COMMAND {
    ICH_HELP = 0, ICH_LIST, ICH_ASSIGN, ICH_RELEASE, ICH_IRQMAP, ICH_NONE
};

/**
 * SystemInfo Table
 */

/**
 * Config Table
 */
#define ICH_COMPOMENTS_NR       4
#define FLAG_VALID         0x0001

struct ParamEntry {
    uint32_t flags;
    uint16_t DSid;
    uint16_t selected;
    uint64_t regbase[ICH_COMPOMENTS_NR];
    int intlines[24];
};

struct cpdev_t;
struct cpdev_ops_t {
    int (*read)(struct cpdev_t *cpdev, char *buf, int len);
    int (*write)(struct cpdev_t *cpdev, char *buf, int len);
    int (*cfgtbl_param_read_qword) 
            (struct cpdev_t *, uint16_t DSid, int rowid, int offset, uint64_t *data);
    int (*cfgtbl_param_write_qword)
            (struct cpdev_t *, uint16_t DSid, int rowid, int offset, uint64_t  data);
    int (*cfgtbl_stat_read_qword)
            (struct cpdev_t *, uint16_t DSid, int rowid, int offset, uint64_t *data);
    int (*cfgtbl_stat_write_qword)
            (struct cpdev_t *, uint16_t DSid, int rowid, int offset, uint64_t  data);
    int (*sysinfo_read)
            (struct cpdev_t *, int base, char *buf, int size);
};

struct cpdev_t {
    int fd;
    struct cpdev_ops_t *ops;
};

int open_ichcp_dev(struct cpdev_t *cpdev, const char *name);
void close_ichcp_dev(struct cpdev_t *cpdev);


// ICH command
int ich_list();
int ich_assign(uint16_t DSid);
int ich_release(uint16_t DSid);
int ich_irqmap(uint16_t DSid, int from ,int to);

#endif	// __ICH_H__
