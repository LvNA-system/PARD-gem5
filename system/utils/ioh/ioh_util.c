#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "ioh.h"

extern const char * IDENT;

static int
iohcp_read(struct cpdev_t *cpdev, char *buf, int len)
{
    return read(cpdev->fd, buf, len);
}

static int
iohcp_write(struct cpdev_t *cpdev, char *buf, int len)
{
    return write(cpdev->fd, buf, len);
}

static int
__iohcp_ioctl(struct cpdev_t *cpdev, int request, struct cp_ioctl_args_t *args)
{
    return ioctl(cpdev->fd, request, args);
}

static int
iohcp_cfgtbl_param_read_qword (
    struct cpdev_t *cpdev,
    int rowid, int offset, uint64_t *data)
{
    struct cp_ioctl_args_t args;
    int ret;

    memset((void *)&args, 9, sizeof(args));
    args.ldom = -1;
    args.addr = 0x00000000 | (rowid&0xFF)<<10 | (offset&0x3FF);
    ret = ioctl(cpdev->fd, CPA_IOCGENTRY, &args);
    if (ret < 0)
        return ret;
    *data = args.value;
    
    return 0;
}

static int
iohcp_cfgtbl_param_write_qword(
    struct cpdev_t *cpdev,
    int rowid, int offset, uint64_t data)
{
    struct cp_ioctl_args_t args;
    int ret;

    memset((void *)&args, 9, sizeof(args));
    args.ldom  = -1;
    args.addr  = 0x00000000 | (rowid&0xFF)<<10 | (offset&0x3FF);
    args.value = data;
    ret = ioctl(cpdev->fd, CPA_IOCSENTRY, &args);

    return ret;
}

static int
iohcp_cfgtbl_stat_read_qword(
    struct cpdev_t *cpdev,
    int rowid, int offset, uint64_t *data)
{
    struct cp_ioctl_args_t args;
    int ret;

    memset((void *)&args, 9, sizeof(args));
    args.ldom = -1;
    args.addr = 0x10000000 | (rowid&0xFF)<<10 | (offset&0x3FF);
    ret = ioctl(cpdev->fd, CPA_IOCGENTRY, &args);
    if (ret < 0)
        return ret;
    *data = args.value;
    
    return 0;
}

static int
iohcp_cfgtbl_stat_write_qword (
    struct cpdev_t *cpdev,
    int rowid, int offset, uint64_t data)
{
    struct cp_ioctl_args_t args;
    int ret;

    memset((void *)&args, 9, sizeof(args));
    args.ldom  = -1;
    args.addr  = 0x10000000 | (rowid&0xFF)<<10 | (offset&0x3FF);
    args.value = data;
    ret = ioctl(cpdev->fd, CPA_IOCSENTRY, &args);
    if (ret < 0)
        return ret;

    return 0;
}

static int
iohcp_sysinfo_read(
    struct cpdev_t *cpdev,
    int base, char *buf, int size)
{
    struct cp_ioctl_args_t args;
    
    assert(base % sizeof(uint64_t) == 0);
    assert(size % sizeof(uint64_t) == 0);

    memset((void *)&args, 0, sizeof(args));

    for (int offset = 0; offset<size; offset+=sizeof(uint64_t)) {
        args.addr = (base + offset) + 0x80000000;
        if (ioctl(cpdev->fd, CPA_IOCGENTRY, &args) != 0)
            return -1;
        *(uint64_t *)(buf + offset) = args.value;
    }

    return size;
}

static struct cpdev_ops_t iohcpdev_ops = {
    .read                     = iohcp_read,
    .write                    = iohcp_write,
    .cfgtbl_param_read_qword  = iohcp_cfgtbl_param_read_qword,
    .cfgtbl_param_write_qword = iohcp_cfgtbl_param_write_qword,
    .cfgtbl_stat_read_qword   = iohcp_cfgtbl_stat_read_qword,
    .cfgtbl_stat_write_qword  = iohcp_cfgtbl_stat_write_qword,
    .sysinfo_read             = iohcp_sysinfo_read,
};

void close_iohcp_dev(struct cpdev_t *cpdev)
{
    close(cpdev->fd);
    cpdev->fd = -1;
    cpdev->ops = NULL;
}

int
open_iohcp_dev(struct cpdev_t *cpdev, const char *filename)
{
    char buf[32];
    int ret;

    // open device file
    if ((cpdev->fd = open(filename, O_RDWR)) == -1) {
        fprintf(stderr, "Error open control plane device \"%s\".\n", filename);
        return errno;
    }
    cpdev->ops = &iohcpdev_ops;

    // read and check control plane IDENT
    ret = cpdev->ops->read(cpdev, buf, 32);
    if (ret < 0 || strncmp(buf, IDENT, strlen(IDENT)) != 0) {
        fprintf(stderr, "%s is not a PARDg5-V IOHub Control Plane\n", filename);
        close_iohcp_dev(cpdev);
        return -EINVAL;
    }

    return 0;
}

