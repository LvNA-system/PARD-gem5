#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "cpa_ioctl.h"

int main(int argc, char *argv[])
{
    char *filename;
    int fd;
    char buf[32];
    struct cpa_ioctl_args_t args;
    int ret;

    if (argc != 2) {
        fprintf(stderr, "usage: %s /dev/cpaXX\n", argv[0]);
        return 0;
    }

    filename = argv[1];
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file for reading");
        exit(EXIT_FAILURE);
    }

    // read and print cpa IDENT
    ret = read(fd, buf, 32);
    if (ret < 0) {
        perror("Error read file");
        exit(EXIT_FAILURE);
    }
    buf[ret+1] = '\0';
    printf("%s: %s\n", filename, buf);

    for (int i=0; i<4; i++) {
/*
        // send CPA_IOCSENTRY ioctl
        memset((void *)&args, 0, sizeof(args));
        args.ldom = 1;
        args.addr = 0x12345678;
        args.value = 0xFFFF0000AAAA0000;
        ret = ioctl(fd, CPA_IOCSENTRY, &args);
        printf("CPA_IOCSENTRY: ret %d\n", ret);
*/
        // send CPA_IOCGENTRY ioctl
        memset((void *)&args, 0, sizeof(args));
        args.ldom = i;
        args.addr = 0;
        ret = ioctl(fd, CPA_IOCGENTRY, &args);
        printf("CPA_IOCGENTRY: ret %d, get: 0x%llx\n", ret, args.value);
    }

    close(fd);

    return 0;
}
