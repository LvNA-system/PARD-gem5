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
#include "ich.h"

const char * IDENT = "PARDg5VICHCP";

struct cpdev_t *cpdev;

static struct option ich_options[] = {
    { "help",    no_argument,       0,  'h' },
    { "list",    no_argument,       0,  'l' },
    { "assign",  no_argument,       0,  'a' },
    { "release", no_argument,       0,  'r' },
    { "irqmap",  no_argument,       0,  'i' },
    { "device",  required_argument, 0,  'd' },
    { "DSid",    required_argument, 0,  's' },
    { "from",    required_argument, 0,  'f' },
    { "to",      required_argument, 0,  't' },
    { 0,         0,                 0,   0  }
};

static const char *ich_cmd_string[] = {
    "--help/-h", "--list/-l", "--assign/-a", "--release/-r", "--irqmap/-i"
};

static const char * helptext =
    "PARDg5-V ICH Device Viewer v1.0 BUILD "__DATE__" "__TIME__"\n\n"
    "  Usage		CommandLine\n"
    "-----------------------------------------------------------------------------\n"
    "  help             ich --help\n"
    "                   ich -h\n"
    "  list all device  ich --device=/dev/cp1 --list\n"
    "                   ich       -d /dev/cp1 -l\n"
    "  assign device    ich --device=/dev/cp1 --assign --DSid=0\n"
    "                   ich       -d /dev/cp1 -a       --DSid=0\n"
    "  release device   ich --device=/dev/cp1 --release --DSid=0\n"
    "                   ich       -d /dev/cp1 -r        --DSid=0\n"
    "  IRQ map          ich --irqmap --from=10 --to=10 --DSid=0\n"
    "                   ich       -d /dev/cp1 -i -f 10 -t 10 --DSid=0\n";


/**
 * All Parameters
 */
enum ICH_COMMAND cmd = ICH_NONE;
int DSid = -1;
const char *devName = NULL;
int irqmap_from = -1;
int irqmap_to = -1;

int parseArgs(int argc, char *argv[])
{
    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "hlarid:s:f:t:", ich_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            if (cmd != ICH_NONE) {
                fprintf(stderr, "Only support one command:\n"
                                "   --help/--list/--assign/--release/--irqmap\n");
                return -EINVAL;
            }
            cmd = ICH_HELP;
            break;
        case 'l':
            if (cmd != ICH_NONE) {
                fprintf(stderr, "Only support one command:\n"
                                "   --help/--list/--assign/--release/--irqmap\n");
                return -EINVAL;
            }
            cmd = ICH_LIST;
            break;
        case 'a':
            if (cmd != ICH_NONE) {
                fprintf(stderr, "Only support one command:\n"
                                "   --help/--list/--assign/--release/--irqmap\n");
                return -EINVAL;
            }
            cmd = ICH_ASSIGN;
            break;
        case 'r':
            if (cmd != ICH_NONE) {
                fprintf(stderr, "Only support one command:\n"
                                "   --help/--list/--assign/--release/--irqmap\n");
                return -EINVAL;
            }
            cmd = ICH_RELEASE;
            break;
        case 'i':
            if (cmd != ICH_NONE) {
                fprintf(stderr, "Only support one command:\n"
                                "   --help/--list/--assign/--release/--irqmap\n");
                return -EINVAL;
            }
            cmd = ICH_IRQMAP;
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
        case 'f':
            if (irqmap_from != -1) {
                fprintf(stderr, "option --from/-f occur multiple times, "
                                "the former one will be ignored.\n");
            }
            irqmap_from = atoi(optarg);
            break;
        case 't':
            if (irqmap_to != -1) {
                fprintf(stderr, "option --to/-t occur multiple times, "
                                "the former one will be ignored.\n");
            }
            irqmap_to = atoi(optarg);
            break;
        case '?':

            break;
        default:
            printf("?? getopt returned character code 0%o ??\n", c);
        }
    }
    if (cmd == ICH_NONE) cmd = ICH_HELP;


    switch (cmd) {
      case ICH_HELP:
        puts(helptext);
        exit(0);
        return 0;
      case ICH_IRQMAP:
        if (irqmap_from == -1) {
            fprintf(stderr, "error: option --from/-f is required for command %s\n",
                    ich_cmd_string[cmd]);
            return -EINVAL;
        }
        if (irqmap_to == -1) {
            fprintf(stderr, "error: option --to/-t is required for command %s\n",
                    ich_cmd_string[cmd]);
            return -EINVAL;
        }
      case ICH_ASSIGN:
      case ICH_RELEASE:
        if (DSid == -1) {
            fprintf(stderr, "error: option --DSid/-s is required for command %s\n",
                    ich_cmd_string[cmd]);
            return -EINVAL;
        }
      case ICH_LIST:
        if (devName == NULL) {
            fprintf(stderr, "error: option --device/-d is required for command %s\n",
                    ich_cmd_string[cmd]);
            return -EINVAL;
        }
        break;
    }

    return 0;
}


/**
 *   Usage		CommandLine
 * -----------------------------------------------------------------------------
 *   help		ich --help
 *                      ich -h
 *
 *   list all device	ich --device=/dev/cp1 --list
 *                      ich       -d /dev/cp1 -l
 *
 *   assign device	ich --device=/dev/cp1 --assign --DSid=0 00:06.0 00:07.0
 *                      ich       -d /dev/cp1 -a           -s 0 00:06.0 00:07.0
 *
 *   release device	ich --device=/dev/cp1 --release --DSid=0 00:07.0
 *                  	ich       -d /dev/cp1 -r            -s 0 00:07.0
 *
 *   IRQ map            ich --device=/dev/cp1 -irqmap --DSid=0 --from=10 --to=10
 *                      ich       -d=/dev/cp1 -i          -s 0     -f 10   -t 10
 */
int main(int argc, char *argv[])
{
    int ret = 0;

    assert (sizeof(struct ParamEntry) % sizeof(uint64_t) == 0);

    // parse and check args
    ret = parseArgs(argc, argv);
    if (ret != 0)
        return ret;

    // open cp device
    cpdev = (struct cpdev_t *)malloc(sizeof(struct cpdev_t));
    if ((ret=open_ichcp_dev(cpdev, devName)) < 0) {
        free(cpdev);
        return ret;
    }

    // run command
    switch (cmd) {
    case ICH_LIST:
        ret = ich_list();
        break;
    case ICH_ASSIGN:
        ret = ich_assign(DSid);
        break;
    case ICH_RELEASE:
        ret = ich_release(DSid);
        break;
    case ICH_IRQMAP:
        ret = ich_irqmap(DSid, irqmap_from, irqmap_to);
    default:
        break;
    }
    if (ret < 0)
        fprintf(stderr, "error: %s\n", strerror(-ret));

    // close device
    close_ichcp_dev(cpdev);
    free(cpdev);

    return ret;
}

