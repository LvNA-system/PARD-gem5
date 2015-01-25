#include "cpa.h"

static void mmap_cpa_open(struct vm_area_struct *vma)
{
    printk(KERN_INFO "mmap_cpa_open()\n");
    /* nothing. */
}
static void mmap_cpa_close(struct vm_area_struct *vma)
{
    printk(KERN_INFO "mmap_cpa_close()\n");
    /*
     * unsigned long  pfn = vma->vm_pgoff;
     * unsigned long  len = vma->vm_end - vma->vm_start;
     *      pgprot_t prot = vma->vm_page_prot;
     */
    /* nothing. architectures can override. */
}

static int mmap_cpa_fault(struct vm_area_struct *vma,
                                   struct vm_fault *vmf)
{
    printk(KERN_INFO "mmap_cpa_fault()\n");
    return 0;
}

static int mmap_cpa_access(struct vm_area_struct *vma, unsigned long addr,
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
    printk(KERN_INFO "mmap_cpa_access(addr=0x%lx, buf=%p, len=%d, write=%d)\n",
           addr, buf, len, write);
    if (!write)
        memset(buf, 'A', len);
    return len;
}

/*
 * vm operation structure for CPA mmap pages
 */
static struct vm_operations_struct mmap_cpa_ops = {
    .open  = mmap_cpa_open,
    .close = mmap_cpa_close,
    .fault = mmap_cpa_fault,
    .access = mmap_cpa_access,
};


/**
 * CPA file operations
 **/
static int
cpa_open (struct inode *inode, struct file *filp)
{
    struct control_plane_adaptor *adaptor;
    struct control_plane *cp;
    struct list_head *pos, *adaptor_pos;
    dev_t devno;

    devno = MKDEV(imajor(filp->f_path.dentry->d_inode),
          iminor(filp->f_path.dentry->d_inode));

    list_for_each(adaptor_pos, &g_adaptor_list) {
        adaptor = list_entry(adaptor_pos, struct control_plane_adaptor, next);
        list_for_each(pos, &adaptor->cp_list) {
            cp = list_entry(pos, struct control_plane, next);
            if (cp->devno == devno) {
                filp->private_data = cp;
                return 0;
            }
        }
    }

    return -ENODEV;
}

static ssize_t
cpa_read(struct file *filp, char __user *data, size_t count, loff_t *ppos)
{
    struct control_plane *cp;

    if (*ppos > 0)
    return 0;

    cp = (struct control_plane *) filp->private_data;
    copy_to_user(data, cp->ident, strlen(cp->ident));
    *(data + strlen(cp->ident)) = '\n';
    *(data + strlen(cp->ident)+1) = '\0';
    *ppos = strlen(cp->ident) + 2;
    return *ppos;
}

int __cpa_ioctl_setentry(
    struct control_plane *cp,
    const struct cpa_ioctl_args_t *args)
{
    struct control_plane_adaptor *adaptor = cp->adaptor;

    spin_lock(&adaptor->lock);
    // select CP
    cpa_writel(0, adaptor->cmd_virtual, cp->cpid);
    // send command
    cpa_writew(  CP_LDOMID_OFFSET, adaptor->data_virtual, args->ldom);
    cpa_writel(CP_DESTADDR_OFFSET, adaptor->data_virtual, args->addr);
    cpa_writeq(    CP_DATA_OFFSET, adaptor->data_virtual, args->value);
    cpa_writeb(     CP_CMD_OFFSET, adaptor->data_virtual, CP_CMD_SETENTRY);
    spin_unlock(&adaptor->lock);

    return 0;
}

int __cpa_ioctl_getentry(
    struct control_plane *cp,
    struct cpa_ioctl_args_t *args)
{
    struct control_plane_adaptor *adaptor = cp->adaptor;

    spin_lock(&adaptor->lock);
    // select CP
    cpa_writel(0, adaptor->cmd_virtual, cp->cpid);
    // send command
    cpa_writew(  CP_LDOMID_OFFSET, adaptor->data_virtual, args->ldom);
    cpa_writel(CP_DESTADDR_OFFSET, adaptor->data_virtual, args->addr);
    cpa_writeb(     CP_CMD_OFFSET, adaptor->data_virtual, CP_CMD_GETENTRY);
    // get result
    args->value = cpa_readq(CP_DATA_OFFSET, adaptor->data_virtual);
    spin_unlock(&adaptor->lock);

    return 0;
}

/*
 * Tape device io controls.
 */
static int
cpa_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long data)
{
    struct control_plane *cp;
    struct cpa_ioctl_args_t args;
    int err = 0;
    int retval = 0;

    printk(KERN_INFO "cpa:ioctl\n");
    cp = (struct control_plane *) filp->private_data;

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
        printk(KERN_WARNING "cpa_ioctl(): CPA_IOCRESET unimplement.\n");
        break;

      case CPA_IOCSENTRY:
        // copy args from user to kernel
        if (copy_from_user(&args, (void __user *)data, sizeof(args)) != 0)
            return -EFAULT;
        retval = __cpa_ioctl_setentry(cp, &args);
        break;

      case CPA_IOCGENTRY:
        // copy args from user to kernel
        if (copy_from_user(&args, (void __user *)data, sizeof(args)) != 0)
            return -EFAULT;
        retval = __cpa_ioctl_getentry(cp, &args);
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

static int
cpa_mmap(struct file * file, struct vm_area_struct * vma)
{
    size_t size = vma->vm_end - vma->vm_start;

    printk(KERN_INFO "cpa_mmap(): <0x%lx, %ld> => 0x%lx\n", vma->vm_pgoff, size, vma->vm_start);

    if (vma->vm_pgoff !=0 || size!=PAGE_SIZE)
        return -EINVAL;

    vma->vm_ops = &mmap_cpa_ops;
    vma->vm_flags |= (VM_RESERVED|VM_IO);
    vma->vm_ops->open(vma);

    return 0;
}


/*
 * file operation structure for CPA device
 */
const struct file_operations cpa_device_fops =
{
    .owner = THIS_MODULE,
    .open  = cpa_open,
    .read  = cpa_read,
    .ioctl = cpa_ioctl,
    .mmap  = cpa_mmap,
};

