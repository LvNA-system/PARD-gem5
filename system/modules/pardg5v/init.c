#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <cpa.h>
#include "pardg5v.h"
#include "ctrlplane.h"

/**
 * global data
 */
static struct kset *g_cpa_kset;

// Declare in cpfops.c
extern        dev_t  g_cp_first_devno;		
extern struct class *g_cp_device_class;


/**
 * control plane device driver
 */
static int
pardg5v_cp_probe(struct control_plane_device *dev, const struct control_plane_id *id)
{
    struct control_plane_op *cpop = get_control_plane_op(dev->type);
    if (!cpop) {
        printk(KERN_ERR "pardg5v: probe: cannot get ops for control plane type '%c'.",
               dev->type);
        return -ENODEV;
    }
    return cpop->probe(dev, cpop->args);
}

static void
pardg5v_cp_remove(struct control_plane_device *dev)
{
    struct control_plane_op *cpop = get_control_plane_op(dev->type);
    if (cpop)
        cpop->remove(dev, cpop->args);
}

static struct control_plane_id pardg5v_cp_tbl [] = {
    {CP_VENDOR_ID_FSG, CP_DEVICE_ID_PARDG5V, 'S'},	/* PARDg5VSysCP */
    {CP_VENDOR_ID_FSG, CP_DEVICE_ID_PARDG5V, 'C'},	/* CacheCP      */
    {CP_VENDOR_ID_FSG, CP_DEVICE_ID_PARDG5V, 'M'},	/* MemCP        */
    {CP_VENDOR_ID_FSG, CP_DEVICE_ID_PARDG5V, 'B'},	/* BridgeCP     */
    {0,}
};

static struct control_plane_driver pardg5v_cp_driver = {
    .name     = PARDG5V_MODULE_NAME,
    .id_table = pardg5v_cp_tbl,
    .probe    = pardg5v_cp_probe,
    .remove   = __devexit_p(pardg5v_cp_remove),
};


/**
 * PARDg5-V Control Plane Driver module init
 */
static int __init
pardg5v_init_module(void)
{
    int ret = 0;

    printk(KERN_INFO "PARDg5-V Control Plane Driver, version "
           DRIVER_VERSION ", " __TIME__ " " __DATE__ "\n");

    /*
     * Create a kset with the name of "cpa", located under /sys/
     */
    g_cpa_kset = kset_create_and_add("cpa", NULL, NULL);
    if (!g_cpa_kset)
        return -ENOMEM;

    /*
     * Register g_cp_device_class device class.
     * this is required for udev to auto create device file.
     */
    g_cp_device_class = class_create(THIS_MODULE, "cpdev");
    if (IS_ERR(g_cp_device_class)) {
        printk(KERN_ERR "pardg5v: Unable to register g_cp_device_class\n");
        ret = PTR_ERR(g_cp_device_class);
        g_cp_device_class = NULL;
        goto err_cp_device_class_create;
    }

    /*
     * Allocate devno range for control plane device
     */
    if ((ret = alloc_chrdev_region(&g_cp_first_devno, 0, NUMBER_OF_CP_DEVICES,
                                   PARDG5V_MODULE_NAME)) < 0) {
        printk(KERN_ERR "pardg5v: Unable to alloc chrdev region for "
                        "control plane device\n");
        goto err_alloc_device_chrdev_region;
    }

    /*
     * Register Control Plane driver
     */
    ret = register_cp_driver(&pardg5v_cp_driver);
    if (ret) {
        printk(KERN_ERR "pardg5v: Unable to register control plane driver\n");
        goto err_register_cp_driver;
    }

    goto out;

err_register_cp_driver:
    unregister_chrdev_region(g_cp_first_devno, NUMBER_OF_CP_DEVICES);
err_alloc_device_chrdev_region:
    class_destroy(g_cp_device_class);
err_cp_device_class_create:
    kset_unregister(g_cpa_kset);
out:
    return ret;
}

static void __exit
pardg5v_cleanup_module(void)
{
    // unregister control plane driver
    unregister_cp_driver(&pardg5v_cp_driver);
    // destroy cp_device_class
    class_destroy(g_cp_device_class);
    // unregister char device devno region
    unregister_chrdev_region(g_cp_first_devno, NUMBER_OF_CP_DEVICES);
    // unregister global cpa kset
    kset_unregister(g_cpa_kset);
}

module_init(pardg5v_init_module);
module_exit(pardg5v_cleanup_module);
