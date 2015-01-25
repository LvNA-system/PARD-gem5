#ifndef __PARDg5V_CPA_H__
#define __PARDg5V_CPA_H__

#include <linux/device.h>
#include <linux/list.h>

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


/**
 * API for control plane driver
 */
int __must_check __register_cp_driver(struct control_plane_driver *, struct module *,
                                      const char *mod_name);
#define register_cp_driver(driver)             \
        __register_cp_driver(driver, THIS_MODULE, KBUILD_MODNAME)

void unregister_cp_driver (struct control_plane_driver *);


int cpn_bus_read_config_qword(struct control_plane_bus *bus,
                              struct control_plane_adaptor *adaptor,
                              uint16_t cpid, uint16_t ldom, uint32_t addr,
                              uint64_t *value);
int cpn_bus_write_config_qword(struct control_plane_bus *bus,
                              struct control_plane_adaptor *adaptor,
                              uint16_t cpid, uint16_t ldom, uint32_t addr,
                              uint64_t value);
int cpn_bus_write_command(struct control_plane_bus *bus,
                          struct control_plane_adaptor *adaptor,
                          uint8_t cmd,
                          uint16_t cpid, uint16_t ldom, uint32_t addr,
                          uint64_t value);

static inline int cpn_read_config_qword(struct control_plane_device *dev,
                                        uint16_t ldom, uint32_t addr, uint64_t *value)
{
    return cpn_bus_read_config_qword(dev->adaptor->bus, dev->adaptor,
                                     dev->cpid, ldom, addr, value);
}

static inline int cpn_write_config_qword(struct control_plane_device *dev,
                                        uint16_t ldom, uint32_t addr, uint64_t value)
{
    return cpn_bus_write_config_qword(dev->adaptor->bus, dev->adaptor,
                                     dev->cpid, ldom, addr, value);
}

static inline int cpn_write_command(struct control_plane_device *dev, uint8_t cmd,
                                    uint16_t ldom, uint32_t addr, uint64_t value)
{
    return cpn_bus_write_command(dev->adaptor->bus, dev->adaptor, cmd,
                                     dev->cpid, ldom, addr, value);
}


#endif	// __PARDg5V_CPA_H__

