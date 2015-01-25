#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/uaccess.h>

#include "cpa.h"
#include "pardg5v.h"
#include "cpa_ioctl.h"

/**
 * global data: allocated in init.c
 */
dev_t g_cp_first_devno;		
struct class *g_cp_device_class;

static int cp_cdev_nr = 0;
int alloc_cp_devno(dev_t *devno, int *seqno)
{
    if (cp_cdev_nr >= NUMBER_OF_CP_DEVICES)
        return -ENOMEM;
    *devno = g_cp_first_devno + cp_cdev_nr;
    *seqno = cp_cdev_nr;
    cp_cdev_nr ++;
    return 0;
}

struct device * cp_device_create(dev_t devno, int seqno)
{
    return device_create(g_cp_device_class, NULL, devno, NULL,
                         CP_CHRDEV_NAME_FMT, seqno);
}
void cp_device_destroy(dev_t devno)
{
    device_destroy(g_cp_device_class, devno);
}

static void mmap_cp_open(struct vm_area_struct *vma)
{
    printk(KERN_INFO "mmap_cp_open()\n");
    /* nothing. */
}
static void mmap_cp_close(struct vm_area_struct *vma)
{
    printk(KERN_INFO "mmap_cp_close()\n");
    /*
     * unsigned long  pfn = vma->vm_pgoff;
     * unsigned long  len = vma->vm_end - vma->vm_start;
     *      pgprot_t prot = vma->vm_page_prot;
     */
    /* nothing. architectures can override. */
}

static int mmap_cp_fault(struct vm_area_struct *vma,
                                   struct vm_fault *vmf)
{
    printk(KERN_INFO "mmap_cp_fault()\n");
    return 0;
}

static int mmap_cp_access(struct vm_area_struct *vma, unsigned long addr,
                           void *buf, int len, int write)
{
/*
    resource_size_t phys_addr;
    unsigned long prot = 0;
    void *maddr;
    int offset = addr & (PAGE_SIZE-1);

    if (!(vma->vm_flags & (VM_IO | VM_PFNMAP)))
        return -EINVAL;

    phys_addr = follow_phys(vma, addr, write, &prot);

    if (!phys_addr)
        return -EINVAL;

    maddr = ioremap_prot(phys_addr, PAGE_SIZE, prot);
    if (write)
        memcpy_toio(maddr + offset, buf, len);
    else
        memcpy_fromio(buf, maddr + offset, len);
    iounmap(maddr);
*/
    printk(KERN_INFO "mmap_cp_access(addr=0x%lx, buf=%p, len=%d, write=%d)\n",
           addr, buf, len, write);
    if (!write)
        memset(buf, 'A', len);
    return len;
}

/*
 * vm operation structure for CPA mmap pages
 */
static struct vm_operations_struct mmap_cp_ops = {
    .open  = mmap_cp_open,
    .close = mmap_cp_close,
    .fault = mmap_cp_fault,
    .access = mmap_cp_access,
};


/**
 * CPA file operations
 **/
int
generic_cp_open (struct inode *inode, struct file *filp)
{
    return -ENODEV;
}

ssize_t
generic_cp_read(struct file *filp, char __user *data, size_t count, loff_t *ppos)
{
    struct control_plane_device *cp;

    if (*ppos > 0)
    return 0;

    cp = (struct control_plane_device *) filp->private_data;
    copy_to_user(data, cp->ident, strlen(cp->ident));
    *(data + strlen(cp->ident)) = '\n';
    *(data + strlen(cp->ident)+1) = '\0';
    *ppos = strlen(cp->ident) + 2;
    return *ppos;
}



/*
 * Tape device io controls.
 */
int
generic_cp_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long data)
{
    struct control_plane_device *cp;
    struct cp_ioctl_args_t args;
    int err = 0;
    int retval = 0;

    cp = (struct control_plane_device *) filp->private_data;

    // check cmd no
    if (_IOC_TYPE(cmd) != CPA_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > CPA_IOC_MAXNR) return -ENOTTY;

    // check user space access right
    if (_IOC_DIR(cmd) & _IOC_READ) {
        err = !access_ok(VERIFY_READ|VERIFY_WRITE,
                         (void __user *)data, _IOC_SIZE(cmd));
    }
    else if (_IOC_DIR(cmd) &_IOC_WRITE) {
        err = !access_ok(VERIFY_READ,
                         (void __user *)data, _IOC_SIZE(cmd));
    }
    if (err)
        return -EFAULT;

    // process cmd
    switch (cmd) {
      case CPA_IOCRESET:
        printk(KERN_WARNING "cp_ioctl(): CPA_IOCRESET unimplement.\n");
        break;

      case CPA_IOCCMD:
        // copy args from user to kernel
        if (copy_from_user(&args, (void __user *)data, sizeof(args)) != 0)
            return -EFAULT;
        retval = cpn_write_command(cp, args.cmd, args.ldom, args.addr, args.value);
        break;

      case CPA_IOCSENTRY:
        // copy args from user to kernel
        if (copy_from_user(&args, (void __user *)data, sizeof(args)) != 0)
            return -EFAULT;
        retval = cpn_write_config_qword(cp, args.ldom, args.addr, args.value);
        break;

      case CPA_IOCGENTRY:
        // copy args from user to kernel
        if (copy_from_user(&args, (void __user *)data, sizeof(args)) != 0)
            return -EFAULT;
        retval = cpn_read_config_qword(cp, args.ldom, args.addr, &args.value);
        if (retval != 0)
            break;
        if (copy_to_user((void __user *)data, &args, sizeof(args)) != 0)
            return -EFAULT;
        break;

      default:
        return -ENOTTY;
    }

    return 0;
}

int
generic_cp_mmap(struct file * file, struct vm_area_struct * vma)
{
    size_t size = vma->vm_end - vma->vm_start;

    printk(KERN_INFO "cp_mmap(): <0x%lx, %ld> => 0x%lx\n", vma->vm_pgoff, size, vma->vm_start);

    if (vma->vm_pgoff !=0 || size!=PAGE_SIZE)
        return -EINVAL;

    vma->vm_ops = &mmap_cp_ops;
    vma->vm_flags |= (VM_RESERVED|VM_IO);
    vma->vm_ops->open(vma);

    return 0;
}

