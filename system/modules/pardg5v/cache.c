#include "ctrlplane.h"
#include "cpa_ioctl.h"

/**
 * Builtin cache ldom attributes in "/sys/cpa/cpaX/ldom/ldomX/______"
 **/

#define __CACHE_LDOM_ATTR(_name, _mode) \
    __GENERIC_LDOM_ATTR(_name, __CPA_OFFSET_OF(struct CPA_CACHE_IOCADDR, _name), _mode)

// Statstics attributes
static struct ldom_attribute __stats_attributes[] = {
    __CACHE_LDOM_ATTR(  capacity, 0444),
    __CACHE_LDOM_ATTR( hit_count, 0444),
    __CACHE_LDOM_ATTR(miss_count, 0444),
};

static struct attribute *stats_attrs[] = {
    &__stats_attributes[0].attr, &__stats_attributes[1].attr,
    &__stats_attributes[2].attr,
    NULL,
};

// Parameters attributes
static struct ldom_attribute __params_attributes[] = {
    __CACHE_LDOM_ATTR(waymask, 0666),
};
static struct attribute *params_attrs[] = {
    &__params_attributes[0].attr,
    NULL,
};

// Triggers attributes
static struct attribute *triggers_attrs[] = {
    NULL,
};

// Build generic control plane operations
static struct generic_control_plane_extra extras = {
    .stats_attrs = stats_attrs,
    .params_attrs = params_attrs,
    .triggers_attrs = triggers_attrs,
};
struct control_plane_op cache_cp_op = GENERIC_CONTROL_PLANE_OP("cache_cp", 'C', &extras);

