#include "cpa.h"

/**
 * global data
 **/
// linked-list adaptor list
struct list_head g_adaptor_list;


/**
 * control plane network bus type
 */
static int
cpn_bus_match(struct device *dev, struct device_driver *driver)
{
    struct control_plane_device *cp_dev = to_cp_device(dev);
    struct control_plane_driver *cp_drv = to_cp_driver(driver);
    const struct control_plane_id *ids = cp_drv->id_table;

    if (ids) {
        while (ids->vendor) {
            if (ids->class == cp_dev->type)
                return 1;
            ids++;
        }
    }
    return 0;
}

static int
cpn_bus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    struct control_plane_device *cp_dev;

    if (!dev)
        return -ENODEV;
   
    cp_dev = to_cp_device(dev);
    if (!cp_dev)
        return -ENODEV;

    if (add_uevent_var(env, "CPNBUS_VERSION=%s", dev_name(&cp_dev->dev)))
        return -ENOMEM;

    return 0;
}

struct control_plane_device *cp_dev_get(struct control_plane_device *dev)
{
    if (dev)
        get_device(&dev->dev);
    return dev;
}
void cp_dev_put(struct control_plane_device *dev)
{
    if (dev)
        put_device(&dev->dev);
}

static int
__cpn_bus_probe(struct control_plane_driver *drv,
                struct control_plane_device *cp_dev)
{
    int error = 0;
    if (!cp_dev->driver && drv->probe) {
        error = drv->probe(cp_dev, NULL/*TODO: fill or remove ID*/);
        if (error >= 0) {
            cp_dev->driver = drv;
            error = 0;
        }
    }
    return error;
}

static int
cpn_bus_probe(struct device *dev)
{
    int error = 0;
    struct control_plane_device *cp_dev;
    struct control_plane_driver *drv;

    drv = to_cp_driver(dev->driver);
    cp_dev = to_cp_device(dev);
    cp_dev_get(cp_dev);
    error = __cpn_bus_probe(drv, cp_dev);
    if (error)
        cp_dev_put(cp_dev);

    return error;
}

static int
cpn_bus_remove(struct device *dev)
{
    struct control_plane_device *cp_dev = to_cp_device(dev);
    struct control_plane_driver *drv = cp_dev->driver;

    if (drv) {
        if (drv->remove)
            drv->remove(cp_dev);
        cp_dev->driver = NULL;
    }

    cp_dev_put(cp_dev);
    return 0;
}

struct bus_type cpn_bus_type = {
    .name   = "cpn",
    .match  = cpn_bus_match,
    .uevent = cpn_bus_uevent,
    .probe  = cpn_bus_probe,
    .remove = cpn_bus_remove,
};

/**
 * Control Plane Driver Register
 */
int __register_cp_driver(struct control_plane_driver *drv, struct module *owner,
                         const char *mod_name)
{
    /* initialize common driver fields */
    drv->driver.name = drv->name;
    drv->driver.bus = &cpn_bus_type;
    drv->driver.owner = owner;
    drv->driver.mod_name = mod_name;

    return driver_register(&drv->driver);
}
void unregister_cp_driver(struct control_plane_driver *driver)
{
    driver_unregister(&driver->driver);
}
EXPORT_SYMBOL(__register_cp_driver);
EXPORT_SYMBOL(unregister_cp_driver);


/**
 * Control Plane Device helper
 */
static void cp_dev_release(struct device *dev) { }
static int
__alloc_cp_device(struct control_plane_device **pcp,
    struct control_plane_adaptor *adaptor,
    uint32_t cp_type, uint16_t cpid)
{
    struct control_plane_device *cp_dev = NULL;
    int err = 0;

    // create struct control_plane
    cp_dev = kzalloc(sizeof(struct control_plane_device), GFP_KERNEL);
    if (!cp_dev) {
        printk(KERN_ERR "cpa: out of memory\n");
        return -ENOMEM;
    }

    // init control plane device
    INIT_LIST_HEAD(&cp_dev->bus_list);
    cp_dev->adaptor = adaptor;
    cp_dev->cpid = cpid;
    cp_dev->type = cp_type;
    *(uint32_t *)(&cp_dev->ident[0]) = cpa_readl(4, adaptor->data_virtual);
    *(uint64_t *)(&cp_dev->ident[4]) = cpa_readq(8, adaptor->data_virtual);
    cp_dev->dev.init_name = kasprintf(GFP_KERNEL, CP_DEV_NAME_FMT,
                                      adaptor->bus->number,
                                      cpid);
    cp_dev->dev.bus = &cpn_bus_type;
    cp_dev->dev.parent = &adaptor->bus->dev;
    cp_dev->dev.release = cp_dev_release;
    cp_dev->driver = NULL;

    // register control plane device
    err = device_register(&cp_dev->dev);
    if (err) {
        printk(KERN_ERR "cpa: error register device\n");
        kfree(cp_dev);
        return err;
    }

    // return allocated 
    *pcp = cp_dev;

    return 0;
}

static void
__free_control_plane(struct control_plane_device *cp_dev)
{
    device_unregister(&cp_dev->dev);
    kfree(cp_dev);
}

/**
 * Control Plane Network Bus helper
 */
static void
cpn_bus_release(struct device *dev)
{ }

static struct control_plane_bus *
__alloc_cpn_bus(int number)
{
    struct control_plane_bus *cpn_bus;
    int ret;

    ret = -ENOMEM;
    if ((cpn_bus = kzalloc(sizeof(struct control_plane_bus), GFP_KERNEL)) == NULL) {
        printk(KERN_ERR "cpn: out of memory\n");
        goto err_allocate_cpn_bus;
    }
    INIT_LIST_HEAD(&cpn_bus->devices);
    cpn_bus->number = number;
    snprintf(cpn_bus->name, 48, CP_BUS_NAME_FMT, number);
    cpn_bus->dev.init_name = cpn_bus->name;
    cpn_bus->dev.release = cpn_bus_release;
    cpn_bus->ops = NULL;

    ret = device_register(&cpn_bus->dev);
    if (ret) {
        printk(KERN_ERR "Unable to register bus cpn%d, err: %d\n", number, ret);
        goto err_device_register;
    }

    return cpn_bus;

err_device_register:
    kfree(cpn_bus);
err_allocate_cpn_bus:
    return NULL;
}

static void
__free_cpn_bus(struct control_plane_bus *cpn_bus)
{
    device_unregister(&cpn_bus->dev);
    kfree(cpn_bus);
}


/**
 * Probe control plane adaptor, caller shoud acquire adaptor->lock
 **/
static int
__probe_control_planes_unsafe(struct control_plane_adaptor *adaptor)
{
    int err;
    int cpid;
    uint32_t cp_type;
    struct control_plane_device *cp_dev = NULL;

    // probe control planes, add it to adaptor->bus
    for (cpid = 0; cpid<NUMBER_OF_CPA_DEVICES; cpid++) {
        // set target CP-ID
        cpa_writel(0, adaptor->cmd_virtual, cpid);
        // Readin CP-Type
        cp_type = cpa_readl(0, adaptor->data_virtual);
        // Ignore invalid CP-Type
        if (cp_type == 0xFFFFFFFF)
            continue;

        // allocate control plane device
        err = __alloc_cp_device(&cp_dev, adaptor, cp_type, cpid);
        if (err) {
            printk(KERN_ERR "cpa: can not alloc control plane %d type 0x%x('%c')\n",
                   cpid, cp_type, cp_type);
            goto err_probe_control_plane;
        }
  
        // add control plane device to adaptor->bus->devices
        list_add_tail(&cp_dev->bus_list, &adaptor->bus->devices);
        printk(KERN_INFO "cpa: cpa_probe: PARDg5-V Control Plane %s type 0x%x'%c'.\n",
               cp_dev->ident, cp_dev->type, cp_dev->type);
    }

    goto out;

err_probe_control_plane:
    {
        struct control_plane_device *cp;
        struct list_head *tmp, *pos;
        list_for_each_safe(pos, tmp, &adaptor->bus->devices) {
            cp = list_entry(pos, struct control_plane_device, bus_list);
            list_del(&cp->bus_list);
            __free_control_plane(cp);
        }
    }

out:
    return err;
}

static int __devinit
cpa_adaptor_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
    static int adaptor_nr = 0;
    int err;
    int bars;
    unsigned long mmio_cmd_start, mmio_cmd_len;
    unsigned long mmio_data_start, mmio_data_len;

    struct control_plane_adaptor *adaptor = NULL;

    // select bars & enable device
    bars = pci_select_bars(dev, IORESOURCE_MEM | IORESOURCE_IO);
    err = pci_enable_device(dev);
    if (err) {
        printk(KERN_ERR "cpa: couldn't enable device!\n");
        return err;
    }

    // Reserve selected PCI I/O and memory resource
    err = pci_request_selected_regions(dev, bars, CPA_MODULE_NAME);
    if (err)
        goto err_pci_reg;
    mmio_cmd_start  = pci_resource_start(dev, 0);
    mmio_cmd_len    = pci_resource_len(dev, 0);
    mmio_data_start = pci_resource_start(dev, 1);
    mmio_data_len   = pci_resource_len(dev, 1);

    // allocate&initialize global control_plane_adaptor
    err = -ENOMEM;
    if ((adaptor = kzalloc(sizeof(struct control_plane_adaptor), GFP_KERNEL)) == NULL)
    {
        printk(KERN_ERR "cpa: out of memory\n");
        goto err_allocate_cpa;
    }
    spin_lock_init(&adaptor->lock);
    adaptor->dev = dev;
    adaptor->bars = bars;
    INIT_LIST_HEAD(&adaptor->node);
    // add to global adaptor list
    list_add_tail(&adaptor->node, &g_adaptor_list);

    // remap I/O to kernel address space
    err = -EIO;
    if (!(adaptor->cmd_virtual = ioremap_nocache(mmio_cmd_start, mmio_cmd_len)))
        goto err_ioremap_cmd;
    if (!(adaptor->data_virtual = ioremap_nocache(mmio_data_start, mmio_data_len)))
        goto err_ioremap_data;

    // register control plane network bus
    adaptor->bus = __alloc_cpn_bus(adaptor_nr);
    if (!adaptor->bus)
        goto err_create_cpn_bus;

    /*
     * Probe all control plane connected to this bus.
     */
    spin_lock(&adaptor->lock);
    err = __probe_control_planes_unsafe(adaptor);
    spin_unlock(&adaptor->lock);
    if (err)
        goto err_probe_control_planes;

    /*
     * Increate ControlPanel Adaptor counter
     */
    adaptor_nr ++;

    goto out;

err_probe_control_planes:
    __free_cpn_bus(adaptor->bus);
err_create_cpn_bus:
    iounmap(adaptor->data_virtual);
err_ioremap_data:
    iounmap(adaptor->cmd_virtual);
err_ioremap_cmd:
    kfree(adaptor);
err_allocate_cpa:
    pci_release_selected_regions(dev, bars);
err_pci_reg:
    pci_disable_device(dev);
out:
    return err;
}


static void
cpa_adaptor_remove(struct pci_dev *dev)
{
    struct control_plane_adaptor *adaptor = NULL;
    struct control_plane_device *cp = NULL;
    struct list_head *tmp, *pos;

    // get control_plane_adaptor pointer from pci_dev
    list_for_each_safe(pos, tmp, &g_adaptor_list) {
        adaptor = list_entry(pos, struct control_plane_adaptor, node);
        if (adaptor->dev == dev)
            goto found_adaptor;
    }
    dev_info(&dev->dev, "try to remove unknow device\n");
    goto out;
    
found_adaptor:
    list_for_each_safe(pos, tmp, &adaptor->bus->devices) {
        cp = list_entry(pos, struct control_plane_device, bus_list);
        list_del(&cp->bus_list);
        __free_control_plane(cp);
    }
    __free_cpn_bus(adaptor->bus);
    iounmap(adaptor->data_virtual);
    iounmap(adaptor->cmd_virtual);
    pci_release_selected_regions(adaptor->dev, adaptor->bars);
    kfree(adaptor);
out:
    pci_disable_device(dev);
}

static struct pci_device_id cpa_pci_tbl [] = {
    {PCI_VENDOR_ID_FSG, PCI_DEVICE_ID_FSG_CPA, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0,}
};

static struct pci_driver cpa_pci_driver = {
    .name     = CPA_MODULE_NAME,
    .id_table = cpa_pci_tbl,
    .probe    = cpa_adaptor_probe,
    .remove   = __devexit_p(cpa_adaptor_remove),
};


/**
 * CPA module init
 **/

static int __init cpa_init_module (void)
{
    int ret;

    printk(KERN_INFO "PARDg5-V Control Panel Adaptor, version "
           DRIVER_VERSION ", " __TIME__ " " __DATE__ "\n");

    /**
     * Register control plane network bus type
     */
    ret = bus_register(&cpn_bus_type);
    if (ret < 0)
        goto err_cpn_bus_register;

    /*
     * Initialize global member
     */
    INIT_LIST_HEAD(&g_adaptor_list);

    /*
     * Register PCI driver
     */
    ret = pci_register_driver(&cpa_pci_driver);
    if (ret) {
        printk(KERN_ERR "cpa: Unable to register pci driver\n");
        goto err_pci_register_driver;
    }

    goto out;

err_pci_register_driver:
    bus_unregister(&cpn_bus_type);
err_cpn_bus_register:
out:
    return ret;
}

static void __exit cpa_cleanup_module (void)
{
    // unregister pci driver
    pci_unregister_driver(&cpa_pci_driver);
    // unregister cpn bus
    bus_unregister(&cpn_bus_type);
}

module_init(cpa_init_module);
module_exit(cpa_cleanup_module);
