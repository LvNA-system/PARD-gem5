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
    {0x0000000000100000, 0x01f00000, 1},	// Memory < 3GB
    {0x0000000002000000, 0xBE000000, 2},	// HOLE between [mem_max, 3GB)
    {0x00000000ffff0000, 0x00010000, 2}		// m5 reserved
};
uint64_t guestE820Nr = 5;

/*
 * SMBios
 */
struct SMBiosBiosInformation {
    uint8_t type;
    uint8_t length;
    uint8_t handleGuest;
    uint8_t reserved;
    uint8_t vendor;
    uint8_t version;
    uint16_t startingAddrSegment;
    uint8_t releaseDate;
    uint8_t romSize;
    uint64_t characteristics;
    uint16_t characteristicExtBytes;
    uint8_t majorVer;
    uint8_t minorVer;
    uint8_t embContFirmwareMajor;
    uint8_t embContFirmwareMinor;
}__attribute__ ((packed));


struct SMBiosTable {
    uint8_t anchorString[4];
    uint8_t checksum;
    uint8_t entryPointLength;
    uint8_t majorVersion;
    uint8_t minorVersion;
    uint16_t maxSize;
    uint8_t entryPointRevision;
    uint8_t formattedArea[5];
    struct {
        uint8_t  anchorString[5];
        uint8_t  checksum;
        uint16_t tableSize;
        uint32_t tableAddr;
        uint16_t numStructs;
        uint8_t  smbiosBCDRevision;
    } intermediateHeader;
    struct SMBiosBiosInformation bios;
}__attribute__ ((packed));

struct SMBiosData {
    struct SMBiosTable smbios;
    char string[20];
}__attribute__((packed));



/*
 * IntelMP
 */


// Size: 20Byte
struct X86IntelMPProcessor {
    uint8_t type;
    uint8_t localApicID;
    uint8_t localApicVersion;
    uint8_t cpuFlags;
    uint32_t cpuSignature;
    uint32_t featureFlags;
    uint64_t reserved;
}__attribute__((packed));

// Size: 8Byte
struct X86IntelMPIOAPIC {
    uint8_t type;
    uint8_t id;
    uint8_t version;
    uint8_t flags;
    uint32_t address;
}__attribute__((packed));

// Size: 8Byte
struct X86IntelMPBus {
    uint8_t type;
    uint8_t busID;
    uint8_t busType[6];
}__attribute__((packed));

// Size: 8Byte
struct X86IntelMPIOIntAssignment {
    uint8_t type;
    uint8_t interruptType;
    uint16_t flags;
    uint8_t sourceBusID;
    uint8_t sourceBusIRQ;
    uint8_t destApicID;
    uint8_t destApicIntIn;
}__attribute__((packed));



// Size:
struct X86IntelMPBusHierarchy {
    uint8_t type;
    uint8_t length;
    uint8_t busID;
    uint8_t info;
    uint8_t parentBus;
    uint8_t reserved[3];
}__attribute__((packed));

struct X86IntelMPHeader {
    uint8_t  signature[4];
    uint16_t offset;
    uint8_t  specRev;
    uint8_t  checksum;
    uint8_t  oemID[8];
    uint8_t  productID[12];
    uint32_t oemTableAddr;
    uint16_t oemTableSize;
    uint16_t baseEntries;
    uint32_t localApic;
    uint16_t extOffset;
    uint8_t  extCheckSum;
    uint8_t  reserved;
}__attribute__((packed));

struct IntelMPTable {
    struct X86IntelMPHeader    header;
    struct X86IntelMPProcessor cpu;
    struct X86IntelMPIOAPIC    io_apic;
    struct X86IntelMPBus       pci_bus;
    struct X86IntelMPBus       isa_bus;
    struct X86IntelMPIOIntAssignment pci_dev4_inta;
    struct {
        struct X86IntelMPIOIntAssignment assign_8259_to_apic;
        struct X86IntelMPIOIntAssignment assign_to_apic;
    } isa_int[15];
    struct X86IntelMPBusHierarchy connect_busses;
}__attribute__((packed));

struct FloatingPointer {
    char signature[4];
    uint32_t tableAddr;
    uint8_t length;
    uint8_t specRev;
    uint8_t checksum;
    uint8_t defaultConfig;
    uint8_t features2_5;
    uint8_t reserved[3];
}__attribute__((packed));

struct EBDA {
    struct SMBiosTable smbiosTable;
    char smbiosString[24];
    struct FloatingPointer mpfp;
    struct IntelMPTable intelMPTable;
}__attribute__((packed));

#define assignISAInt(irq, apicPin) 		\
    {						\
        { 3, 3, 0, 1, irq, 1, 0 },		\
        { 3, 0, 0, 1, irq, 1, apicPin },	\
    }

struct EBDA ebdaData = {
    .smbiosTable = {
        .anchorString = "_SM_",
        .checksum = 0x58,
        .entryPointLength = 0x1F,
        .majorVersion = 2,
        .minorVersion = 5,
        .maxSize = 0x24,
        .entryPointRevision = 0,
        .formattedArea = {0,0,0,0,0},
        .intermediateHeader = {
            .anchorString = "_DMI_",
            .checksum = 0xf0,
            .tableSize = 0x24,
            .tableAddr = 0xf001f,
            .numStructs = 1,
            .smbiosBCDRevision = 0x25,
        },
        .bios = {
            .type = 0,
            .length = 0x18, //sizeof(struct SMBiosBiosInformation)+20,
            .handleGuest = 0,
            .reserved = 0,
            .vendor = 0,
            .version = 0,
            .startingAddrSegment = 0,
            .releaseDate = 1,
            .romSize = 0,
            .characteristics = 0,
            .characteristicExtBytes = 0,
            .majorVer = 0,
            .minorVer = 0,
            .embContFirmwareMajor = 0,
            .embContFirmwareMinor = 0,
        }
    },
    .smbiosString = "06/08/2008\0",//.smbiosString = "FSG\0""1.0a\0""01/07/2015\0",
    .mpfp = {
        .signature = "_MP_",
        .tableAddr = 0xf0060,
        .length = 0x1,
        .specRev = 0x4,
        .checksum = 0xb1,
        .defaultConfig = 0,
        .features2_5 = 0x80,
    },
    .intelMPTable = {
        .header = {
            .signature = "PCMP",
            .offset = 0x140,
            .specRev = 0x4,
            .checksum = 0x58,
            .oemID = "",
            .productID = "",
            .oemTableAddr = 0x0,
            .oemTableSize = 0x0,
            .baseEntries = 0x21,
            .localApic = 0xfee00000,
            .extOffset = 0x8,
            .extCheckSum = 0x75,
            .reserved = 0x0,
        },
        .cpu = {
            .type = 0,
            .localApicID = 0,
            .localApicVersion = 0x14,
            .cpuFlags = 0x3,
            .cpuSignature = 0x0,
            .featureFlags = 0x0,
        },
        .io_apic = {
            .type = 2,
            .id = 1,
            .version = 0x11,
            .flags = 0x1,
            .address = 0xfec00000,
        },
        .pci_bus = {
            .type = 1,
            .busID = 0x0,
            .busType = "PCI",
        },
        .isa_bus = {
            .type = 1,
            .busID = 0x1,
            .busType = "ISA",
        },
        .pci_dev4_inta = {3, 0, 0, 0, 16, 1, 16},
        .isa_int = {
            assignISAInt(0, 2),
            assignISAInt(1, 1),
            assignISAInt(3, 3),
            assignISAInt(4, 4),
            assignISAInt(5, 5),
            assignISAInt(6, 6),
            assignISAInt(7, 7),
            assignISAInt(8, 8),
            assignISAInt(9, 9),
            assignISAInt(10, 10),
            assignISAInt(11, 11),
            assignISAInt(12, 12),
            assignISAInt(13, 13),
            assignISAInt(14, 14),
            assignISAInt(15, 15),
        },
        .connect_busses = {
            .type = 0x81,
            .length = 0x8,
            .busID = 0x1,
            .info = 0x1,
            .parentBus = 0x0,
        },
    },
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

#define ebdaPos        0xF0000
#define realModeData   0x90200
#define e820MapNrPointer (realModeData + 0x1e8)
#define e820MapPointer   (realModeData + 0x2d0)

struct Segment all_segments[] = {
    {e820MapNrPointer, sizeof(guestE820Nr), 0, (uint8_t*)&guestE820Nr},
    {e820MapPointer,   sizeof(e820),        0, (uint8_t*)e820},
    {ebdaPos,          sizeof(ebdaData),    0, (uint8_t*)&ebdaData},
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

    union CPADDR addr;
    addr.data = 0;
    addr.cfgtbl.type = 0;
    addr.cfgtbl.cfgtbl_type = 0;
    addr.cfgtbl.row_id = 0;


    if (argc != 3) {
        fprintf(stderr, "usage: %s /dev/cpXX <DSid>\n", argv[0]);
        return 0;
    }

    filename = argv[1];
    DSid = atoi(argv[2]);

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
    args.ldom = 0xF0F0;
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
    args.ldom = 0xF0F0;
    args.addr = addr.data;
    args.value = 0x0000000000000001;
    ret = ioctl(fd, CPA_IOCSENTRY, &args);
    fprintf(stderr, "Init ParamTable for DSid=0xF0F0: cpuMask 0x%x\n", args.value);


    // paramTable->DSid/VALID
    addr.cfgtbl.offset = 0;
    memset((void *)&args, 0, sizeof(args));
    args.ldom = 0xF0F0;
    args.addr = addr.data;
    args.value = 0x000000008000F0F0;
    ret = ioctl(fd, CPA_IOCSENTRY, &args);
    fprintf(stderr, "Init ParamTable for DSid=0xF0F0\n");

    // paramTable->bsp_entry_addr 0x200000
    

    printf("Send Boot Command.....");
    memset((void *)&args, 0, sizeof(args));
    args.cmd = 'B';
    args.ldom = 0xF0F0;
    ret = ioctl(fd, CPA_IOCCMD, &args);
    printf("OK\n");

    close(fd);

    return 0;
}
