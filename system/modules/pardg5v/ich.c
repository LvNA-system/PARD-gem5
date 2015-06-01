#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#include "cpa.h"
#include "pardg5v.h"
#include "ctrlplane.h"

static LIST_HEAD(g_cp_list);

struct ichcp_device {
    struct list_head node;
    struct control_plane_device *dev;

    /* device file */
    dev_t devno;		// device no
    struct device *sysdev;	// devfile in /sys/class/cp_device/*
    struct cdev cdev;		// chrdev file in /dev/*
};


/**
 * File Operations of Control Plane Device
 *
 *  - most operations are generic excpet "open()"
 *
 */

static int
ich_cp_open (struct inode *inode, struct file *filp)
{
    struct list_head *pos;
    struct ichcp_device *ichcp;
    dev_t devno;

    devno = MKDEV(imajor(filp->f_path.dentry->d_inode),
          iminor(filp->f_path.dentry->d_inode));

    list_for_each(pos, &g_cp_list) {
        ichcp = list_entry(pos, struct ichcp_device, node);
        if (ichcp->devno == devno) {
            filp->private_data = ichcp->dev;
            return 0;
        }
    }
    return -ENODEV;
}

const struct file_operations
ich_cp_device_fops = {
    .owner = THIS_MODULE,
    .open  = ich_cp_open,
    .read  = generic_cp_read,
    .ioctl = generic_cp_ioctl,
    .mmap  = generic_cp_mmap,
};



/**
 * Control Plane Driver Interfaces.
 *
 *  - ich_cp_probe  : called for each control plane device when this driver loaded
 *  - ich_cp_remove : called when driver unload
 *
 */

static int
__allocate_ichcp_device(struct ichcp_device **pichcp,
    struct control_plane_device *dev)
{
    struct ichcp_device *ichcp = NULL;
    int seqno = -1;
    int err = 0;

    /*
     * allocate and initialize ichcp_device
     */
    if ((ichcp = kzalloc(sizeof(struct ichcp_device), GFP_KERNEL)) == NULL) {
        printk(KERN_ERR "pardg5v: out of memory\n");
        return -ENOMEM;
    }
    ichcp->dev = dev;
    ichcp->sysdev = NULL;
    INIT_LIST_HEAD(&ichcp->node);

    /*
     * allocate devno and seqno
     */
    err = alloc_cp_devno(&ichcp->devno, &seqno);
    if (err) {
        printk(KERN_ERR "pardg5v: error allocate devno for control plane "
                        CP_DEV_NAME_FMT"\n",
                        dev->adaptor->bus->number, dev->cpid);
        goto err_alloc_cp_devno;
    }

    /*
     * Create device in "/sys/class/cp_devices/cpXXX"
     */
    ichcp->sysdev = cp_device_create(ichcp->devno, seqno);
    if (IS_ERR(ichcp->sysdev)) {
        err = PTR_ERR(ichcp->sysdev);
        printk(KERN_ERR "pardg5v: Unable to create control plane device "
                        CP_CHRDEV_NAME_FMT"\n", seqno);
        goto err_device_create;
    }

    /*
     * Register a character device as an interface to a user mode
     * program that handles the control plane specific functionality.
     */
    cdev_init(&ichcp->cdev, &ich_cp_device_fops);
    ichcp->cdev.owner = THIS_MODULE;
    err = cdev_add(&ichcp->cdev, ichcp->devno, 1);
    if (err) {
        printk(KERN_ERR "pardg5v: Failed to create char device for "
                        CP_DEV_NAME_FMT"\n",
               dev->adaptor->bus->number, dev->cpid);
        goto err_cdev_add;
    }

    /*
     * return allocated ichcp_device
     */
    *pichcp = ichcp;
    goto out;

err_cdev_add:
    cp_device_destroy(ichcp->devno);
err_device_create:
err_alloc_cp_devno:
    kfree(ichcp);
out:
    return err;
}

static void
__free_ichcp_device(struct ichcp_device *ichcp)
{
    cdev_del(&ichcp->cdev);
    cp_device_destroy(ichcp->devno);
    kfree(ichcp);
}

static int
ich_cp_probe(struct control_plane_device *dev, void *args)
{
    struct ichcp_device *ichcp = NULL;
    int err = 0;

    /*
     * allocate ichcp_device struct for this control plane device
     */
    err = __allocate_ichcp_device(&ichcp, dev);
    if (err)
        return err;

    /*
     * add control plane to g_cp_list
     */
    list_add_tail(&ichcp->node, &g_cp_list);
    printk(KERN_INFO "pardg5v: "CP_DEV_NAME_FMT": PARDg5VICHCP detected.\n",
           dev->adaptor->bus->number, dev->cpid);

    return err;
}

void
ich_cp_remove(struct control_plane_device *dev, void *args)
{
    struct ichcp_device *ichcp = NULL;
    struct list_head *tmp, *pos;

    list_for_each_safe(pos, tmp, &g_cp_list) {
        ichcp = list_entry(pos, struct ichcp_device, node);
        if (ichcp->dev == dev) {
            list_del(&ichcp->node);
            __free_ichcp_device(ichcp);
            return;
        }
    }
    printk(KERN_WARNING "pardg5v: try to remove unknown control plane device "
                        CP_DEV_NAME_FMT"\n",
                        dev->adaptor->bus->number, dev->cpid);
}

struct control_plane_op ich_cp_op = {
    .name = "PARDg5VICHCP",
    .type = 'I',
    .probe  = ich_cp_probe,
    .remove = ich_cp_remove,
    .args = NULL,
};

