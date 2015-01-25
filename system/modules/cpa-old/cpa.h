#ifndef __PARDg5V_CPA_H__
#define __PARDg5V_CPA_H__

#undef __KERNEL__
#define __KERNEL__

#undef MODULE
#define MODULE

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/init.h>		// included for __init and __exit macros
#include <linux/kernel.h>	// included for KERN_INFO
#include <linux/module.h>	// included for all kernel modules
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "cpa_ioctl.h"

MODULE_LICENSE("GPL");

#define CPA_MODULE_NAME "cpa"
#define DRIVER_VERSION "1.0"

#define PCI_VENDOR_ID_FSG        0x0A19
#define PCI_DEVICE_ID_FSG_CPA    0x0001

#define CPID_MASK                0x3f
#define NUMBER_OF_CPA_DEVICES    0x40
#define NUMBER_OF_CPA_ADAPTORS	 0x08

extern struct class *cpa_device_class;
extern dev_t g_cpa_first_devno;
extern struct list_head g_adaptor_list;

#define CPA_ADAPTOR_NAME_FMT	"adaptor%d"
struct control_plane_adaptor {
    /* lock for this struct */
    spinlock_t lock;
    /* pci_dev pointer */
    struct pci_dev *dev;
    /* enabled bars' mask */
    int bars;
    /* BAR0: cmd & BAR1: data */
    u8 __iomem *cmd_virtual;
    u8 __iomem *data_virtual;
    /* file in "/sys" */
    struct device *sysdev;
    /* device file in "/dev" */
    dev_t devno;
    struct cdev cdev;
    /* next control_plane_adaptor */
    struct list_head next;
    /* all control plane list */
    struct list_head cp_list;
};

#define CPA_DEVICE_NAME_FMT "cpa%d"
struct control_plane {
    /* lock for this struct */
    spinlock_t lock;
    /* kobject of control_plane */
    struct kobject kobj;

    /* parent control plane adaptor */
    struct control_plane_adaptor *adaptor;

    /* kset of all ldoms */
    struct kset *kset_ldoms;
    struct control_plane_op *ops;

    /* list of all ldoms */
    struct list_head ldoms_list;

    /* cp probe */
    uint32_t type;	// control plane type
    char ident[13];	// control plane identifier
    uint16_t cpid;	// control plane ID
    /* file in "/sys" */
    struct device *sysdev;
    /* device file */
    dev_t devno;	// device no.
    struct cdev cdev;	// device file

    /* link to next */
    struct list_head next;
};
#define kobj_to_cp(x) container_of(x, struct control_plane, kobj)

/*
 * Register I/O
 */
#define cpa_readb(where, mmio) readb(mmio + where)
#define cpa_readw(where, mmio) readw(mmio + where)
#define cpa_readl(where, mmio) readl(mmio + where)
#define cpa_readq(where, mmio) readq(mmio + where)
#define cpa_writeb(where, mmio, val) writeb(val, mmio + where)
#define cpa_writew(where, mmio, val) writew(val, mmio + where)
#define cpa_writel(where, mmio, val) writel(val, mmio + where)
#define cpa_writeq(where, mmio, val) writeq(val, mmio + where)


/**
 * CPA Address Spaces, ref "hyper/gm/CPAdaptor.cc"
 *
 *   63                 31                 0
 *   +------------------+------------------+  0h
 *   |     IDENT_LOW    |       Type       |
 *   +------------------+------------------+  8h
 *   |                IDENT_HIGH           |
 *   +------------------+--------+----+----+ 10h
 *   |     destAddr     | LDomID |STAT| CMD|
 *   +------------------+--------+----+----+ 18h
 *   |                data                 |
 *   +-------------------------------------+ 20h
 *
 **/
#define CP_TYPE_OFFSET		 0x0
#define CP_IDENT_HIGH_OFFSET	 0x4
#define CP_IDENT_LOW_OFFSET	 0x8
#define CP_CMD_OFFSET		0x10
#define CP_STATE_OFFSET		0x11
#define CP_LDOMID_OFFSET	0x12
#define CP_DESTADDR_OFFSET	0x14
#define CP_DATA_OFFSET		0x18

#define CP_CMD_GETENTRY		'G'
#define CP_CMD_SETENTRY		'S'

#endif	// __PARDg5V_CPA_H__

