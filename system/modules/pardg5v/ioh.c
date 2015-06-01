#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#include "cpa.h"
#include "pardg5v.h"
#include "ctrlplane.h"

static LIST_HEAD(g_cp_list);

struct iohcp_device {
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
ioh_cp_open (struct inode *inode, struct file *filp)
{
    struct list_head *pos;
    struct iohcp_device *iohcp;
    dev_t devno;

    devno = MKDEV(imajor(filp->f_path.dentry->d_inode),
          iminor(filp->f_path.dentry->d_inode));

    list_for_each(pos, &g_cp_list) {
        iohcp = list_entry(pos, struct iohcp_device, node);
        if (iohcp->devno == devno) {
            filp->private_data = iohcp->dev;
            return 0;
        }
    }
    return -ENODEV;
}

const struct file_operations
ioh_cp_device_fops = {
    .owner = THIS_MODULE,
    .open  = ioh_cp_open,
    .read  = generic_cp_read,
    .ioctl = generic_cp_ioctl,
    .mmap  = generic_cp_mmap,
};



/**
 * Control Plane Driver Interfaces.
 *
 *  - ioh_cp_probe  : called for each control plane device when this driver loaded
 *  - ioh_cp_remove : called when driver unload
 *
 */

static int
__allocate_iohcp_device(struct iohcp_device **piohcp,
    struct control_plane_device *dev)
{
    struct iohcp_device *iohcp = NULL;
    int seqno = -1;
    int err = 0;

    /*
     * allocate and initialize iohcp_device
     */
    if ((iohcp = kzalloc(sizeof(struct iohcp_device), GFP_KERNEL)) == NULL) {
        printk(KERN_ERR "pardg5v: out of memory\n");
        return -ENOMEM;
    }
    iohcp->dev = dev;
    iohcp->sysdev = NULL;
    INIT_LIST_HEAD(&iohcp->node);

    /*
     * allocate devno and seqno
     */
    err = alloc_cp_devno(&iohcp->devno, &seqno);
    if (err) {
        printk(KERN_ERR "pardg5v: error allocate devno for control plane "
                        CP_DEV_NAME_FMT"\n",
                        dev->adaptor->bus->number, dev->cpid);
        goto err_alloc_cp_devno;
    }

    /*
     * Create device in "/sys/class/cp_devices/cpXXX"
     */
    iohcp->sysdev = cp_device_create(iohcp->devno, seqno);
    if (IS_ERR(iohcp->sysdev)) {
        err = PTR_ERR(iohcp->sysdev);
        printk(KERN_ERR "pardg5v: Unable to create control plane device "
                        CP_CHRDEV_NAME_FMT"\n", seqno);
        goto err_device_create;
    }

    /*
     * Register a character device as an interface to a user mode
     * program that handles the control plane specific functionality.
     */
    cdev_init(&iohcp->cdev, &ioh_cp_device_fops);
    iohcp->cdev.owner = THIS_MODULE;
    err = cdev_add(&iohcp->cdev, iohcp->devno, 1);
    if (err) {
        printk(KERN_ERR "pardg5v: Failed to create char device for "
                        CP_DEV_NAME_FMT"\n",
               dev->adaptor->bus->number, dev->cpid);
        goto err_cdev_add;
    }

    /*
     * return allocated iohcp_device
     */
    *piohcp = iohcp;
    goto out;

err_cdev_add:
    cp_device_destroy(iohcp->devno);
err_device_create:
err_alloc_cp_devno:
    kfree(iohcp);
out:
    return err;
}

static void
__free_iohcp_device(struct iohcp_device *iohcp)
{
    cdev_del(&iohcp->cdev);
    cp_device_destroy(iohcp->devno);
    kfree(iohcp);
}

static int
ioh_cp_probe(struct control_plane_device *dev, void *args)
{
    struct iohcp_device *iohcp = NULL;
    int err = 0;

    /*
     * allocate iohcp_device struct for this control plane device
     */
    err = __allocate_iohcp_device(&iohcp, dev);
    if (err)
        return err;

    /*
     * add control plane to g_cp_list
     */
    list_add_tail(&iohcp->node, &g_cp_list);
    printk(KERN_INFO "pardg5v: "CP_DEV_NAME_FMT": PARDg5VIOHubCP detected.\n",
           dev->adaptor->bus->number, dev->cpid);

    return err;
}

void
ioh_cp_remove(struct control_plane_device *dev, void *args)
{
    struct iohcp_device *iohcp = NULL;
    struct list_head *tmp, *pos;

    list_for_each_safe(pos, tmp, &g_cp_list) {
        iohcp = list_entry(pos, struct iohcp_device, node);
        if (iohcp->dev == dev) {
            list_del(&iohcp->node);
            __free_iohcp_device(iohcp);
            return;
        }
    }
    printk(KERN_WARNING "pardg5v: try to remove unknown control plane device "
                        CP_DEV_NAME_FMT"\n",
                        dev->adaptor->bus->number, dev->cpid);
}

struct control_plane_op ioh_cp_op = {
    .name = "PARDg5VIOHCP",
    .type = 'H',
    .probe  = ioh_cp_probe,
    .remove = ioh_cp_remove,
    .args = NULL,
};

