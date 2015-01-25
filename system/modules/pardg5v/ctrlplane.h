#ifndef __PARDG5V_CTRLPLANE_H__
#define __PARDG5V_CTRLPLANE_H__

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/uaccess.h>

#include "cpa.h"

struct control_plane_op {
    char type;
    const char *name;
    int  (*probe) (struct control_plane_device *, void *);
    void (*remove)(struct control_plane_device *, void *);
    void *args;
};

struct control_plane_op * get_control_plane_op(char type);

// declare in cpfops.h
int             alloc_cp_devno(dev_t *pdevno, int *seqno);
struct device * cp_device_create(dev_t devno, int seqno);
void cp_device_destroy(dev_t devno);

int     generic_cp_open  (struct inode *, struct file *);
ssize_t generic_cp_read  (struct file *, char __user *, size_t, loff_t *);
int     generic_cp_ioctl (struct inode *, struct file *, unsigned int, unsigned long);
int     generic_cp_mmap  (struct file *, struct vm_area_struct *);

#endif	// __PARDG5V_CTRLPLANE_H__
