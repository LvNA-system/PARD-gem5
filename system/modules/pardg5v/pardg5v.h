#ifndef __PARDg5V_CONTROL_PLANE_DRIVER__
#define __PARDg5V_CONTROL_PLANE_DRIVER__

#include <linux/module.h>	// included for all kernel modules

MODULE_LICENSE("GPL");

#define PARDG5V_MODULE_NAME "pardg5v"
#define DRIVER_VERSION "1.0"

#define CP_VENDOR_ID_FSG	0x0A19
#define CP_DEVICE_ID_PARDG5V	0x0001

#define NUMBER_OF_CP_DEVICES	0x40


#define CP_DEV_NAME_FMT         "0000:%02d:%02d"
#define CP_CHRDEV_NAME_FMT	"cp%d"

#endif	// __PARDg5V_CONTROL_PLANE_DRIVER__
