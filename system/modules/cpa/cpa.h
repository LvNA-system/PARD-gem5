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

MODULE_LICENSE("GPL");

#define CPA_MODULE_NAME "cpa"
#define DRIVER_VERSION "2.0"

#define PCI_VENDOR_ID_FSG        0x0A19
#define PCI_DEVICE_ID_FSG_CPA    0x0001

#define CPID_MASK                0x3f
#define NUMBER_OF_CPA_DEVICES    0x40
#define NUMBER_OF_CPA_ADAPTORS	 0x08


#define CP_BUS_NAME_FMT		"cpn0000:%02d"
#define CP_DEV_NAME_FMT		"0000:%02d:%02d"
//#define CP_ADAPTOR_NAME_FMT     "cpa%d"



extern struct class *cpa_device_class;
extern dev_t g_cpa_first_devno;
extern struct list_head g_adaptor_list;

#define CP_ANY_ID (~0)
struct control_plane_id {
    __u32 vendor, device;	/* Vendor and device ID or CP_ANY_ID */
    __u32 class;                /* class */
};

struct control_plane_adaptor {
    struct list_head   node;         /* node in list of adaptors */
    struct pci_dev    *dev;          /* pci_dev pointer */
    spinlock_t         lock;         /* lock for this struct */
    int                bars;         /* enabled bars' mask */
    u8 __iomem        *cmd_virtual;  /* BAR0: cmd */
    u8 __iomem        *data_virtual; /* BAR1: data */
    struct control_plane_bus *bus;   /* cpn bus this adaptor connected */
};

struct control_plane_bus {
    struct list_head   devices;     /* list of devices on this bus */
    unsigned char      number;      /* bus number */
    char               name[48];    /* bus name */
    struct device      dev;
    struct control_plane_ops *ops;  /* control plane access ops */
};
#define to_cp_bus(n)   container_of(n, struct control_plane_bus, dev)


struct control_plane_device {
    struct list_head bus_list;		/* node in per-bus list */
    struct control_plane_adaptor *adaptor;
                                        /* adaptor connected to this device */
    uint16_t cpid;			/* control plane ID         */
    uint32_t type;			/* control plane type       */
    char ident[13];			/* control plane identifier */

    struct device dev;			/* Generic device interface */
    struct control_plane_driver *driver;/* which driver has allocated this device */
};
#define to_cp_device(dev) container_of(dev, struct control_plane_device, dev)

struct control_plane_driver {
    struct list_head node;
    char *name;
    const struct control_plane_id *id_table;

    int (*probe)(struct control_plane_device *dev, const struct control_plane_id *id);
    void (*remove)(struct control_plane_device *dev);
    struct device_driver driver;
};
#define to_cp_driver(drv)  container_of(drv, struct control_plane_driver, driver)


struct control_plane_bus * create_cpn_bus(int number);
void remove_cpn_bus(struct control_plane_bus *cpn_bus);

int  register_cp_device   (struct control_plane_device *);
void unregister_cp_device (struct control_plane_device *);
int  __register_cp_driver (struct control_plane_driver *drv, struct module *owner,
                           const char *mod_name);
void unregister_cp_driver (struct control_plane_driver *);


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

