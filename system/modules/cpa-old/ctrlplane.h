#include "cpa.h"

struct control_plane_op {
    char type;
    const char *name;
    void (*probe)(struct control_plane *);
    void (*remove)(struct control_plane *);
    void *args;
};


/**
 * LDom attribute type
 **/

struct ldom_attribute {
    struct attribute attr;
    unsigned long iocaddr;
};
#define to_ldom_attr(x) container_of(x, struct ldom_attribute, attr)

#define __GENERIC_LDOM_ATTR(_name, _iocaddr, _mode) {       \
    .attr = {.name = __stringify(_name), .mode = _mode },   \
    .iocaddr = _iocaddr,                                    \
}


/**
 * Control plane operation for generic device
 */

struct generic_control_plane_extra {
    struct attribute **stats_attrs;
    struct attribute **params_attrs;
    struct attribute **triggers_attrs;
};

void generic_cp_probe(struct control_plane *);
void generic_cp_remove(struct control_plane *);
#define GENERIC_CONTROL_PLANE_OP(_name, _type, _args) {    \
    .name   = _name,                                       \
    .type   = _type,                                       \
    .probe  = generic_cp_probe,                            \
    .remove = generic_cp_remove,                           \
    .args   = (void *)_args,                               \
}

extern struct control_plane_op * control_plane_tbl [];

