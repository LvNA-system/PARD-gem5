#include "cpa.h"
#include "ctrlplane.h"

/**
 * global data
 **/
// cpa adaptor class
struct class *cpa_adaptor_class;	
dev_t g_adaptor_first_devno;
// cpa device class
struct class *cpa_device_class;
dev_t g_cpa_first_devno;
// linked-list adaptor list
struct list_head g_adaptor_list;

static struct kset *g_cpa_kset;


/**
 * external data
 **/
extern const struct file_operations cpa_device_fops;
extern struct kobj_type cp_ktype;


/**
 * Forward declare of control plane alloc/free
 **/
static int
__alloc_control_plane(struct control_plane **pcp,
    struct control_plane_adaptor *adaptor,
    uint32_t cp_type, uint16_t cpid)
{
    static int cpdev_nr = 0;
    int err = 0;
    struct control_plane *cp;

    /* 
     * allocate and initialize control_plane
     */
    if ((cp = kzalloc(sizeof(struct control_plane), GFP_KERNEL)) == NULL) {
        err = -ENOMEM;
        printk(KERN_ERR "cpa: out of memory\n");
        return err;
    }
    spin_lock_init(&cp->lock);
    cp->kobj.kset = g_cpa_kset;
    cp->adaptor = adaptor;
    INIT_LIST_HEAD(&cp->ldoms_list);
    cp->type = cp_type;
    memset(cp->ident, 0, sizeof(cp->ident));
    cp->cpid = cpid;
    cp->devno = g_cpa_first_devno + cpdev_nr;
    INIT_LIST_HEAD(&cp->next);
    // control_plane_op
    for (struct control_plane_op **cpop = control_plane_tbl;
         *cpop != NULL; cpop++)
    {
        if ((*cpop)->type == cp->type) {
            cp->ops = *cpop;
            break;
        }
    }

    /*
     * Create device in "/sys/class/cpa_devices/cpaXXX"
     */
    cp->sysdev = device_create(cpa_device_class, NULL, cp->devno, NULL,
                               CPA_DEVICE_NAME_FMT, cpdev_nr);
    if (IS_ERR(cp->sysdev)) {
        err = PTR_ERR(cp->sysdev);
        printk(KERN_ERR "cpa: Unable to create cpa device "
                        CPA_DEVICE_NAME_FMT"\n", cpdev_nr);
        goto err_device_create;
    }

    /*
     * Register a character device as an interface to a user mode
     * program that handles the CPA specific functionality.
     */
    cdev_init(&cp->cdev, &cpa_device_fops);  
    cp->cdev.owner = THIS_MODULE;  
    err = cdev_add(&cp->cdev, cp->devno, 1);
    if (err) {
        printk(KERN_ERR "cpa: Failed to create char device "CPA_DEVICE_NAME_FMT"\n",
               cpdev_nr);
        goto err_cdev_add;
    }

    /*
     * Initialize and add the kobject to /sys/cpa/cpaX
     */
    err = kobject_init_and_add(&cp->kobj, &cp_ktype, NULL,
                               CPA_DEVICE_NAME_FMT, cpdev_nr);
    if (err) {
        printk(KERN_ERR "cpa: Unable to add kobject for "
                        CPA_DEVICE_NAME_FMT"\n", cpdev_nr);
        goto err_kobject_init_and_add;
    }
    // create "/sys/cpa/cpaX/adaptor" symbol link to parent adaptor
    err = sysfs_create_link(&cp->kobj, &adaptor->sysdev->kobj, "adaptor");
    if (err) {
        printk(KERN_ERR "cpa: Unable to create symbol link to parent adaptor for "
                        CPA_DEVICE_NAME_FMT"\n", cpdev_nr);
        goto err_sysfs_create_link_adaptor;
    }
    // create "/sys/cpa/cpaX/device" symbol link to self device class
    err = sysfs_create_link(&cp->kobj, &cp->sysdev->kobj, "device");
    if (err) {
        printk(KERN_ERR "cpa: Unable to create symbol link to device for "
                        CPA_DEVICE_NAME_FMT"\n", cpdev_nr);
        goto err_sysfs_create_link_device;
    }
    // create "/sys/cpa/cpaX/ldoms" kset for all LDOMs
    cp->kset_ldoms = kset_create_and_add("ldoms", NULL, &cp->kobj);
    if (!cp->kset_ldoms) {
        printk(KERN_ERR "cpa: Unable to create ldoms kset for "
                        CPA_DEVICE_NAME_FMT"\n", cpdev_nr);
        goto err_kset_create_and_add;
    }

    /*
     * Increate ControlPanel device counter
     */
    cpdev_nr++;

    *pcp = cp;
    goto out;

err_kset_create_and_add:
    sysfs_remove_link(&cp->kobj, "device");
err_sysfs_create_link_device:
    sysfs_remove_link(&cp->kobj, "adaptor");
err_sysfs_create_link_adaptor:
    kobject_put(&cp->kobj);
err_kobject_init_and_add:
    cdev_del(&cp->cdev);
err_cdev_add:
    device_destroy(cpa_device_class, cp->devno);
err_device_create:
    kfree(cp);
out:
    return err;
}

static void
__free_control_plane(struct control_plane *cp)
{
    //printk(KERN_INFO "cpa: release control plane %s\n", cp->ident);

    // call control_plane specific remove first
    if (cp->ops)
        cp->ops->remove(cp);

    kset_unregister(cp->kset_ldoms);
    sysfs_remove_link(&cp->kobj, "device");
    sysfs_remove_link(&cp->kobj, "adaptor");
    kobject_put(&cp->kobj);
    cdev_del(&cp->cdev);
    device_destroy(cpa_device_class, cp->devno);
    kfree(cp);
}


/**
 * ControlPlane Adaptor Ops: "/dev/adaptorX"
 **/
static ssize_t
cpa_adaptor_read(struct file *file, char __user *buf, size_t count,
                loff_t *ppos)
{
    char *str = "PARDg5-V Control Panel Adaptor, version "
                DRIVER_VERSION ", " __TIME__ " " __DATE__ "\n";
    if (*ppos > 0)
        return 0;
    copy_to_user(buf,str,strlen(str));
    *(buf + strlen(str)) = '\0';
    *ppos = strlen(str)+1;
    return strlen(str)+1;
}

static const struct file_operations cpa_adaptor_fops = {
    .owner = THIS_MODULE,
    .read = cpa_adaptor_read,
};


/**
 * PCI Driver Data
 **/

static int
__probe_control_planes_unsafe(struct control_plane_adaptor *adaptor)
{
    int err;
    int cpid;
    uint32_t cp_type;
    struct control_plane *cp = NULL;

    // probe control planes, create /dev/cpaXX for each CP
    for (cpid = 0; cpid<NUMBER_OF_CPA_DEVICES; cpid++) {
        // set target CP-ID
        cpa_writel(0, adaptor->cmd_virtual, cpid);
        // Readin CP-Type
        cp_type = cpa_readl(0, adaptor->data_virtual);
        // Ignore invalid CP-Type
        if (cp_type == 0xFFFFFFFF)
            continue;

        // create struct control_plane
        err = __alloc_control_plane(&cp, adaptor, cp_type, cpid);
        if (err) {
            printk(KERN_ERR "cpa: can not alloc control plane %d type 0x%x('%c')\n",
                   cpid, cp_type, cp_type);
            goto err_alloc_control_plane;
        }

        // init control_plane
        *(uint32_t *)(&cp->ident[0]) = cpa_readl(4, adaptor->data_virtual);
        *(uint64_t *)(&cp->ident[4]) = cpa_readq(8, adaptor->data_virtual);

        // add to cpa->cp_list
        list_add_tail(&cp->next, &adaptor->cp_list);

        printk(KERN_INFO "cpa: cpa_probe: PARDg5-V Control Plane %s type 0x%x'%c'.\n",
               cp->ident, cp->type, cp->type);

        /*
         * probe this control_plane
         */
        if (cp->ops)
            cp->ops->probe(cp);
    }

    goto out;

err_alloc_control_plane:
    {
        struct control_plane *cp;
        struct list_head *tmp, *pos;
        list_for_each_safe(pos, tmp, &adaptor->cp_list) {
            cp = list_entry(pos, struct control_plane, next);
            list_del(&cp->next);
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
    adaptor->devno = g_adaptor_first_devno + adaptor_nr;
    INIT_LIST_HEAD(&adaptor->cp_list);
    INIT_LIST_HEAD(&adaptor->next);
    // add to global adaptor list
    list_add_tail(&adaptor->next, &g_adaptor_list);

    // remap I/O to kernel address space
    err = -EIO;
    if (!(adaptor->cmd_virtual = ioremap_nocache(mmio_cmd_start, mmio_cmd_len)))
        goto err_ioremap_cmd;
    if (!(adaptor->data_virtual = ioremap_nocache(mmio_data_start, mmio_data_len)))
        goto err_ioremap_data;

    /*
     * Create root device (/dev/cpa) for driver
     */
    adaptor->sysdev = device_create(cpa_adaptor_class, NULL, adaptor->devno, NULL,
                                    CPA_ADAPTOR_NAME_FMT, adaptor_nr);
    if (IS_ERR(adaptor->sysdev)) {
        err = PTR_ERR(adaptor->sysdev);
        printk(KERN_ERR "cpa: Unable to create cpa adaptor device, err: %d\n", err);
        goto err_device_create;
    }

    /*
     * Register a character device as an interface to a user mode
     * program that handles the CPA specific functionality.
     */
    cdev_init(&adaptor->cdev, &cpa_adaptor_fops);  
    adaptor->cdev.owner = THIS_MODULE;  
    err = cdev_add(&adaptor->cdev, adaptor->devno, 1);
    if (err) {
        printk(KERN_ERR "Failed to open char device\n");
        goto err_cdev_add;
    }

    /*
     * Probe all cpa connected to this adaptor.
     */
    spin_lock(&adaptor->lock);
    err = __probe_control_planes_unsafe(adaptor);
    spin_unlock(&adaptor->lock);
    if (err) {
        printk(KERN_ERR "Failed to probe control planes\n");
        goto err_probe_control_planes;
    }

    /*
     * Increate ControlPanel Adaptor counter
     */
    adaptor_nr ++;

    goto out;

err_probe_control_planes:
    cdev_del(&adaptor->cdev);
err_cdev_add:
    device_destroy(cpa_adaptor_class, adaptor->devno);
err_device_create:
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
    struct control_plane *cp = NULL;
    struct list_head *tmp, *pos;

    // get control_plane_adaptor pointer from pci_dev
    list_for_each_safe(pos, tmp, &g_adaptor_list) {
        adaptor = list_entry(pos, struct control_plane_adaptor, next);
        if (adaptor->dev == dev)
            goto found_adaptor;
    }
    dev_info(&dev->dev, "try to remove unknow device\n");
    goto out;
    
found_adaptor:
    list_for_each_safe(pos, tmp, &adaptor->cp_list) {
        cp = list_entry(pos, struct control_plane, next);
        list_del(&cp->next);
        __free_control_plane(cp);
    }

    cdev_del(&adaptor->cdev);
    device_destroy(cpa_adaptor_class, adaptor->devno);
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
    .name = CPA_MODULE_NAME,
    .id_table = cpa_pci_tbl,
    .probe = cpa_adaptor_probe,
    .remove = __devexit_p(cpa_adaptor_remove),
};



/**
 * CPA module init
 **/

static int __init cpa_init_module (void)
{
    int ret;

    printk(KERN_INFO "PARDg5-V Control Panel Adaptor, version "
           DRIVER_VERSION ", " __TIME__ " " __DATE__ "\n");

    /*
     * Create a kset with the name of "cpa", located under /sys/
     */
    g_cpa_kset = kset_create_and_add("cpa", NULL, NULL);
    if (!g_cpa_kset)
        return -ENOMEM;


    /*
     * Initialize global member
     */
    INIT_LIST_HEAD(&g_adaptor_list);


    /*
     * Register device class: cpa_adaptor & cpa_device
     */
    cpa_adaptor_class = class_create(THIS_MODULE, "cpa_adaptor");
    if(IS_ERR(cpa_device_class)) {
        printk(KERN_ERR "cpa: Unable to register cpa_adaptor class\n");
        ret = PTR_ERR(cpa_adaptor_class);
        cpa_adaptor_class = NULL;
        goto err_adaptor_class_create;
    }
    cpa_device_class = class_create(THIS_MODULE, "cpa_device");
    if(IS_ERR(cpa_device_class)) {
        printk(KERN_ERR "cpa: Unable to register cpa_device class\n");
        ret = PTR_ERR(cpa_device_class);
        cpa_device_class = NULL;
        goto err_device_class_create;
    }

    /*
     * Alocate devno range
     */
    if ((ret = alloc_chrdev_region(&g_adaptor_first_devno, 0, NUMBER_OF_CPA_ADAPTORS,
                                   CPA_MODULE_NAME)) < 0) {
        printk(KERN_ERR "cpa: Unable to alloc chrdev region for cpa_adaptor\n");
        goto err_alloc_adaptor_chrdev_region;
    }
    if ((ret = alloc_chrdev_region(&g_cpa_first_devno, 0, NUMBER_OF_CPA_DEVICES,
                                   CPA_MODULE_NAME)) < 0) {
        printk(KERN_ERR "cpa: Unable to alloc chrdev region for cpa_device\n");
        goto err_alloc_device_chrdev_region;
    }

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
    unregister_chrdev_region(g_cpa_first_devno, NUMBER_OF_CPA_DEVICES);
err_alloc_device_chrdev_region:
    unregister_chrdev_region(g_adaptor_first_devno, NUMBER_OF_CPA_ADAPTORS);
err_alloc_adaptor_chrdev_region:
    class_destroy(cpa_device_class);
err_device_class_create:
    class_destroy(cpa_adaptor_class);
err_adaptor_class_create:
out:
    return ret;
}

static void __exit cpa_cleanup_module (void)
{
    // unregister pci driver
    pci_unregister_driver(&cpa_pci_driver);
    // unregister char device devno region
    unregister_chrdev_region(g_cpa_first_devno, NUMBER_OF_CPA_DEVICES);
    unregister_chrdev_region(g_adaptor_first_devno, NUMBER_OF_CPA_ADAPTORS);
    // destroy device class
    class_destroy(cpa_device_class);
    class_destroy(cpa_adaptor_class);
    // unregister global cpa kset
    kset_unregister(g_cpa_kset);
}

module_init(cpa_init_module);
module_exit(cpa_cleanup_module);
