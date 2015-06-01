#include <assert.h>
#include <getopt.h>
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
#include "ioh.h"

const char * IDENT = "PARDg5VIOHCP";

struct cpdev_t *cpdev;

/**
 * Forward declare
 */
int queryDevices(struct cpdev_t *cpdev, struct DEVICE **ppFirstDev);


static struct option ioh_options[] = {
    { "help",    no_argument,       0,  'h' },
    { "list",    no_argument,       0,  'l' },
    { "assign",  no_argument,       0,  'a' },
    { "release", no_argument,       0,  'r' },
    { "device",  required_argument, 0,  'd' },
    { "DSid",    required_argument, 0,  's' },
    { 0,         0,                 0,   0  }
};

static const char *ioh_cmd_string[] = {
    "--help/-h", "--list/-l", "--assign/-a", "--release/-r", ""
};

static const char * helptext =
    "PARDg5-V IOHub Device Viewer v1.0 BUILD "__DATE__" "__TIME__"\n\n"
    "  Usage		CommandLine\n"
    "-----------------------------------------------------------------------------\n"
    "  help             ioh --help\n"
    "                   ioh -h\n"
    "  list all device  ioh --device=/dev/cp1 --list\n"
    "                   ioh       -d /dev/cp1 -l\n"
    "  assign device    ioh --device=/dev/cp1 --assign --DSid=0 00:06.0 00:07.0\n"
    "                   ioh       -d /dev/cp1 -a       --DSid=0 00:06.0 00:07.0\n"
    "  release device   ioh --device=/dev/cp1 --release --DSid=0 00:07.0\n"
    "                   ioh       -d /dev/cp1 -r        --DSid=0 00:07.0\n";

static uint16_t parsePciID(const char *pciid)
{
    int bus, dev, fun;
    int ret;

    ret = sscanf(pciid, "%d:%d.%d", &bus, &dev, &fun);
    return (ret == 3) ? ((bus&0xFF)<<8|(dev&0xF)<<4|(fun&0xF)) : 0xFFFF;
}

/**
 * All Parameters
 */
enum IOH_COMMAND cmd = IOH_NONE;
int DSid = -1;
const char *devName = NULL;
uint16_t pciids[32];
int pciids_nr = 0;

int parseArgs(int argc, char *argv[])
{
    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "hlard:s:", ioh_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            if (cmd != IOH_NONE) {
                fprintf(stderr, "Only support one command:\n"
                                "   --help/--list/--assign/--release\n");
                return -EINVAL;
            }
            cmd = IOH_HELP;
            break;
        case 'l':
            if (cmd != IOH_NONE) {
                fprintf(stderr, "Only support one command:\n"
                                "   --help/--list/--assign/--release\n");
                return -EINVAL;
            }
            cmd = IOH_LIST;
            break;
        case 'a':
            if (cmd != IOH_NONE) {
                fprintf(stderr, "Only support one command:\n"
                                "   --help/--list/--assign/--release\n");
                return -EINVAL;
            }
            cmd = IOH_ASSIGN;
            break;
        case 'r':
            if (cmd != IOH_NONE) {
                fprintf(stderr, "Only support one command:\n"
                                "   --help/--list/--assign/--release\n");
                return -EINVAL;
            }
            cmd = IOH_RELEASE;
            break;
        case 's':
            if (DSid != -1) {
                fprintf(stderr, "option --DSid/-s occur multiple times,"
                                "the former one will be ignored.\n");
            }
            DSid = atoi(optarg);
            break;
        case 'd':
            if (devName) {
                fprintf(stderr, "Multiple devices was not allowed.\n");
                return -EINVAL;
            }
            devName = optarg;
            break;
        case '?':
            break;
        default:
            printf("?? getopt returned character code 0%o ??\n", c);
        }
    }
    if (cmd == IOH_NONE) cmd = IOH_HELP;

    if (optind < argc) {
        while (optind < argc) {
            if ((pciids[pciids_nr] = parsePciID(argv[optind])) == 0xFFFF) {
                printf("Error device %s\n", argv[optind]);
                return -EINVAL;
            }
            optind++;
            pciids_nr++;
        }
    }

    switch (cmd) {
      case IOH_HELP:
        puts(helptext);
        exit(0);
        return 0;
      case IOH_ASSIGN:
        if (pciids_nr == 0) {
            fprintf(stderr, "error: pci device id are required for command %s\n",
                    ioh_cmd_string[cmd]);
            return -EINVAL;
        }
      case IOH_RELEASE:
        if (DSid == -1) {
            fprintf(stderr, "error: option --DSid/-s is required for command %s\n",
                    ioh_cmd_string[cmd]);
            return -EINVAL;
        }
      case IOH_LIST:
        if (devName == NULL) {
            fprintf(stderr, "error: option --device/-d is required for command %s\n",
                    ioh_cmd_string[cmd]);
            return -EINVAL;
        }
        break;
    }

    return 0;
}


/**
 *   Usage		CommandLine
 * -----------------------------------------------------------------------------
 *   help		ioh --help
 *                      ioh -h
 *
 *   list all device	ioh --device=/dev/cp1 --list
 *                      ioh       -d /dev/cp1 -l
 *
 *   assign device	ioh --device=/dev/cp1 --assign --DSid=0 00:06.0 00:07.0
 *                      ioh       -d /dev/cp1 -a       --DSid=0 00:06.0 00:07.0
 *
 *   release device	ioh --device=/dev/cp1 --release --DSid=0 00:07.0
 *                  	ioh       -d /dev/cp1 -r        --DSid=0 00:07.0
 */
int main(int argc, char *argv[])
{
    int ret = 0;

    // parse and check args
    ret = parseArgs(argc, argv);
    if (ret != 0)
        return ret;

    // open cp device
    cpdev = (struct cpdev_t *)malloc(sizeof(struct cpdev_t));
    if ((ret=open_iohcp_dev(cpdev, devName)) < 0) {
        free(cpdev);
        return ret;
    }

    // query all devices
    struct DEVICE *devices;
    ret = queryDevices(cpdev, &devices);

    // run command
    switch (cmd) {
    case IOH_LIST:
        ret = ioh_list(devices);
        break;
    case IOH_ASSIGN:
        ret = ioh_assign(devices, DSid, pciids, pciids_nr);
        break;
    case IOH_RELEASE:
        ret = ioh_release(devices, DSid, pciids, pciids_nr);
        break;
    default:
        break;
    }

    // close device
    close_iohcp_dev(cpdev);
    free(cpdev);

    return ret;
}
       

static int fixupDevices(struct DEVICE *devices)
{
    int row = 0;
    struct ParamEntry param;
    while (1) {
        // read param entry data
        cpdev->ops->cfgtbl_param_read_qword(cpdev, row++, 0, (uint64_t *)&param);
        // EOF detected
        if (*((uint64_t *)&param) == (uint64_t)0xFFFFFFFFFFFFFFFF)
            break;
        // VALID entry
        if (param.flags & FLAG_VALID) {
            struct DEVICE *pDev = devices;
            while (pDev != NULL) {
                if (param.device_mask & (1<<pDev->info.id)) {
                    pDev->guests[pDev->guests_nr] = param.DSid;
                    pDev->guests_nr ++;
                }
                pDev = pDev->next;
            }
        }
    }
}

int
queryDevices(struct cpdev_t *cpdev, struct DEVICE **ppFirstDev)
{
    struct DEVICE *pDev = NULL;
    struct DEVICE *pLast = NULL;
    struct IOHubInfo iohinfo;
    int ret;

    *ppFirstDev = NULL;

    // Query IOHubInfo
    ret = cpdev->ops->sysinfo_read(cpdev, 0, (char *)&iohinfo, sizeof(iohinfo));
    if (ret != sizeof(iohinfo)) {
        fprintf(stderr, "Error reading IOHubInfo.\n");
        return -1;
    }

    int pos = iohinfo.device_ptr;
    // Query all devices
    while(pos != 0xFFFF) {
        pDev = (struct DEVICE *)malloc(sizeof(struct DEVICE));
        memset(pDev, 0, sizeof(struct DEVICE));
        if (!pDev ||
            cpdev->ops->sysinfo_read(cpdev, pos, (char *)&pDev->info,
                                     sizeof(pDev->info)) < 0)
            goto err;
        pos = pDev->info.next_ptr;

        if (pLast == NULL) {
            pDev->next = NULL;
            *ppFirstDev = pLast = pDev;
        }
        else {
            pDev->next = pLast->next;
            pLast->next = pDev;
            pLast = pDev;
        }
    }

    // Fixup DEVICE field
    return fixupDevices(*ppFirstDev);

err:
    while ((pDev = *ppFirstDev) != NULL) {
        *ppFirstDev = pDev->next;
        free(pDev);
    }
    return -1;
}

