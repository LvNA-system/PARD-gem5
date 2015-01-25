#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "cpa_ioctl.h"

const char * IDENT = "PARDg5vSysCP";


/*
 * E820
 */
struct e820entry {
    uint64_t addr;
    uint64_t size;
    uint32_t type;
}__attribute__ ((packed));

struct e820entry e820[] = {
    {0x0000000000000000, 0x0009fc00, 1},	// LowMem
    {0x000000000009fc00, 0x00060400, 2},	// BIOS Reserved
    {0x0000000000100000, 0x7ff00000, 1},	// Memory < 3GB
    {0x0000000080000000, 0x40000000, 2},	// HOLE between [mem_max, 3GB)
    {0x00000000ffff0000, 0x00010000, 2}		// m5 reserved
};
uint64_t guestE820Nr = 5;

uint16_t ebdaData[] = {
    0x535f, 0x5f4d, 0x1f58, 0x0502, 0x0024, 0x0000, 0x0000, 0x0000,
    0x445f, 0x494d, 0xf05f, 0x0024, 0x001f, 0x000f, 0x0001, 0x0025,
    0x0018, 0x0000, 0x0000, 0x0100, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x3000, 0x2f36, 0x3830, 0x322f, 0x3030,
    0x0038, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x4d5f, 0x5f50, 0x0060, 0x000f, 0x0401, 0x00b1, 0x0080, 0x0000,
    0x4350, 0x504d, 0x0140, 0x6204, 0x5346, 0x0047, 0x0000, 0x0000,
    0x4150, 0x4452, 0x3567, 0x562d, 0x722e, 0x0030, 0x0000, 0x0000,
    0x0000, 0x0021, 0x0000, 0xfee0, 0x0008, 0x0075, 0x0000, 0x0314,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0102, 0x0111, 0x0000, 0xfec0, 0x0001, 0x4350, 0x0049, 0x0000,
    0x0101, 0x5349, 0x0041, 0x0000, 0x0003, 0x0000, 0x1000, 0x1001,
    0x0303, 0x0000, 0x0001, 0x0001, 0x0003, 0x0000, 0x0001, 0x0201,
    0x0303, 0x0000, 0x0101, 0x0001, 0x0003, 0x0000, 0x0101, 0x0101,
    0x0303, 0x0000, 0x0301, 0x0001, 0x0003, 0x0000, 0x0301, 0x0301,
    0x0303, 0x0000, 0x0401, 0x0001, 0x0003, 0x0000, 0x0401, 0x0401,
    0x0303, 0x0000, 0x0501, 0x0001, 0x0003, 0x0000, 0x0501, 0x0501,
    0x0303, 0x0000, 0x0601, 0x0001, 0x0003, 0x0000, 0x0601, 0x0601,
    0x0303, 0x0000, 0x0701, 0x0001, 0x0003, 0x0000, 0x0701, 0x0701,
    0x0303, 0x0000, 0x0801, 0x0001, 0x0003, 0x0000, 0x0801, 0x0801,
    0x0303, 0x0000, 0x0901, 0x0001, 0x0003, 0x0000, 0x0901, 0x0901,
    0x0303, 0x0000, 0x0a01, 0x0001, 0x0003, 0x0000, 0x0a01, 0x0a01,
    0x0303, 0x0000, 0x0b01, 0x0001, 0x0003, 0x0000, 0x0b01, 0x0b01,
    0x0303, 0x0000, 0x0c01, 0x0001, 0x0003, 0x0000, 0x0c01, 0x0c01,
    0x0303, 0x0000, 0x0d01, 0x0001, 0x0003, 0x0000, 0x0d01, 0x0d01,
    0x0303, 0x0000, 0x0e01, 0x0001, 0x0003, 0x0000, 0x0e01, 0x0e01,
    0x0881, 0x0101, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

/*
 * All Segments
 */
struct Segment {
    uint64_t base;
    uint32_t size; 
    uint32_t offset;
    uint8_t *ptr;
};

#define realModeData   0x90200
#define e820MapNrPointer (realModeData + 0x1e8)
#define e820MapPointer   (realModeData + 0x2d0)
#define ebdaPos        0xF0000

struct Segment all_segments[] = {
    {e820MapNrPointer, sizeof(guestE820Nr), 0, (uint8_t*)&guestE820Nr},
    {e820MapPointer,   sizeof(e820),        0, (uint8_t*)e820        },
    {ebdaPos,          sizeof(ebdaData),    0, (uint8_t*)ebdaData    },
    {0, 0, 0, NULL}
};


union CPADDR {
    uint32_t data;
    struct {
        unsigned offset      : 10;
        unsigned row_id      :  8;
        unsigned reserved    : 10;
        unsigned cfgtbl_type :  2;
        unsigned type        :  2;
    } cfgtbl;
    struct {
        unsigned offset      : 24;
        unsigned reserved    :  6;
        unsigned type        :  2;
    } cfgmem;
    struct {
        unsigned offset      :  8;
        unsigned reserved    : 22;
        unsigned type        :  2;
    } sysinfo;
};


int
write_config_memory(int fd, int base, const char *buf, int size)
{
    union CPADDR addr;
    struct cp_ioctl_args_t args;

    assert(size % sizeof(uint64_t) == 0);

    addr.data = 0;
    addr.cfgmem.type = 1;
    memset((void *)&args, 0, sizeof(args));

    for (int offset = 0; offset<size; offset+=sizeof(uint64_t)) {
        addr.cfgmem.offset = base + offset;
        args.addr = addr.data;
        args.value = *(uint64_t *)(buf + offset);
        if (ioctl(fd, CPA_IOCSENTRY, &args) != 0)
            return -1;
    }

    return size;
}


int main(int argc, char *argv[])
{
    char *filename;
    short DSid;
    int fd;
    char buf[32];
    struct cp_ioctl_args_t args;
    int ret;

    printf("PARDg5-V BIOS Loader v1.0 BUILD "__DATE__" "__TIME__"\n\n");

    // Check&Display Segments
    struct Segment *seg = all_segments;
    while (seg->size != 0) {
        if (seg->size%8 != 0)
            seg->size = ((seg->size+7)/8) * 8;
        assert(seg->size % 8 == 0);
        printf("Seg: 0x%x, 0x%x\n", seg->base, seg->size);
        seg++;
    }


    if (argc != 3) {
        fprintf(stderr, "usage: %s /dev/cpXX <DSid>\n", argv[0]);
        return 0;
    }

    filename = argv[1];
    DSid = atoi(argv[2]);

    if (DSid >=4 ) {
        fprintf(stderr, "Invalid args: DSid must be less than 4.\n");
        return -EINVAL;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file for reading");
        exit(EXIT_FAILURE);
    }

    // read and check control plane IDENT
    ret = read(fd, buf, 32);
    if (ret < 0 || strncmp(buf, IDENT, strlen(IDENT)) != 0) {
        fprintf(stderr, "%s is not a PARDg5-V System Control Plane", filename);
        exit(EXIT_FAILURE);
    }

    // Get Available ConfigTable Entries
    union CPADDR addr;
    addr.data = 0;
    addr.cfgtbl.type = 0;
    addr.cfgtbl.cfgtbl_type = 0;
    addr.cfgtbl.row_id = 0;
    addr.cfgtbl.offset = 0;
    while (1) {
        memset((void *)&args, 0, sizeof(args));
        args.ldom = DSid;
        args.addr = addr.data;
        ioctl(fd, CPA_IOCGENTRY, &args);
        // Found EOF
        if (args.value == 0xFFFFFFFFFFFFFFFF) {
            fprintf(stderr, "Found EOF in ParamTable!\n");
            return -ENOMEM;
        }
        // Found empty entry
        if (!(args.value & 0x80000000))
            break;
        fprintf(stderr, "%d ==> DSid#%d\n", addr.cfgtbl.row_id, args.value & 0xFFFF);
        addr.cfgtbl.row_id ++;
    }


    // Write Config Memory
    seg = all_segments;
    uint32_t offset = 0;
    while (seg->size != 0) {
        seg->offset = offset;
        offset += write_config_memory(fd, offset, seg->ptr, seg->size);
        seg++;
    }

    // paramTable->segments
    seg = all_segments;
    addr.cfgtbl.offset = 24;
    args.ldom = DSid;
    while (seg->size != 0) {
        args.addr = addr.data;
        args.value = seg->base;
        ret = ioctl(fd, CPA_IOCSENTRY, &args);
        addr.cfgtbl.offset += 8;
        args.addr = addr.data;
        args.value = ((uint64_t)seg->offset << 32) | seg->size;
        ret = ioctl(fd, CPA_IOCSENTRY, &args);
        addr.cfgtbl.offset += 8;
        seg++;
    }

    // paramTable->cpuMask
    addr.cfgtbl.offset = 8;
    memset((void *)&args, 0, sizeof(args));
    args.ldom = DSid;
    args.addr = addr.data;
    args.value = 1<<DSid;
    ret = ioctl(fd, CPA_IOCSENTRY, &args);
    fprintf(stderr, "Init ParamTable for DSid=0x%x: cpuMask 0x%x\n", args.ldom, args.value);


    // paramTable->DSid/VALID
    addr.cfgtbl.offset = 0;
    memset((void *)&args, 0, sizeof(args));
    args.ldom = DSid;
    args.addr = addr.data;
    args.value = 0x0000000080000000 | (DSid & 0xFFFF);
    ret = ioctl(fd, CPA_IOCSENTRY, &args);
    fprintf(stderr, "Init ParamTable for DSid=0x%x\n", DSid);

    // paramTable->bsp_entry_addr 0x200000
    

    printf("Send Boot Command.....");
    memset((void *)&args, 0, sizeof(args));
    args.cmd = 'B';
    args.ldom = DSid;
    ret = ioctl(fd, CPA_IOCCMD, &args);
    printf("OK\n");

    close(fd);

    return 0;
}
