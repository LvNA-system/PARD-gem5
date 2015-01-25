#include "cpa.h"
#include "ctrlplane.h"
#include "cpa_ioctl.h"

/**
 * Forward function declaration, defined in "cpa/main.c"
 **/

extern int __cpa_ioctl_getentry(struct control_plane *, struct cpa_ioctl_args_t *);
extern int __cpa_ioctl_setentry(struct control_plane *, const struct cpa_ioctl_args_t *);


/**
 * Structure defination
 **/

struct ldom_obj;
struct ldom_sub {
    struct kobject kobj;
    struct kobj_type ktype;
    struct ldom_obj *ldom_obj;
};
struct ldom_obj {
    struct kset *kset;
    struct control_plane *cp;
    LDomID ldomID;
    struct ldom_sub stats;
    struct ldom_sub params;
    struct ldom_sub triggers;
    struct list_head next;
};
#define to_ldom_obj(x) (container_of(x, struct ldom_sub, kobj)->ldom_obj)


/**
 * Declaration of sysfs operations
 **/

static ssize_t ldom_attr_show(struct kobject *kobj,
                              struct attribute *attr,
                              char *buf)
{
    struct ldom_attribute *attribute;
    struct ldom_obj *ldom;
    struct cpa_ioctl_args_t args;
    int err;

    attribute = to_ldom_attr(attr);
    ldom = to_ldom_obj(kobj);

    args.ldom = ldom->ldomID;
    args.addr = attribute->iocaddr;
    err = __cpa_ioctl_getentry(ldom->cp, &args);
    if (err)
        return err;

    if (args.value == -1)
        return sprintf(buf, "INV\n");
    else
        return sprintf(buf, "%lx\n", args.value);
}

static ssize_t ldom_attr_store(struct kobject *kobj,
                               struct attribute *attr,
                               const char *buf, size_t len)
{
    struct ldom_attribute *attribute;
    struct ldom_obj *ldom;
    struct cpa_ioctl_args_t args;
    int err;

    attribute = to_ldom_attr(attr);
    ldom = to_ldom_obj(kobj);

    args.ldom = ldom->ldomID;
    args.addr = attribute->iocaddr;
    if (sscanf(buf, "%lx", &args.value) != 1)
        return -EINVAL;
    err = __cpa_ioctl_setentry(ldom->cp, &args);
    if (err)
        return err;
    
    return len;
}

static struct sysfs_ops ldom_sysfs_ops = {
    .show = ldom_attr_show,
    .store = ldom_attr_store,
};

static void ldom_release(struct kobject *kobj)
{
    //printk(KERN_INFO "cap: ldom_release called with %s\n", kobj->name);
    //struct ldom_obj *ldom;
    //ldom = to_ldom_obj(kobj);
    //kfree(ldom);
}


/**
 * Declaration of control plane operations
 */

void generic_cp_probe(struct control_plane *cp)
{
    struct cpa_ioctl_args_t args;
    struct generic_control_plane_extra *extra =
        (struct generic_control_plane_extra *)cp->ops->args;
    int ret;

    printk(KERN_INFO "cpa: %s: ", cp->ident);
    for (int i=0; i<256; i++) {
        args.ldom = i;
        args.addr = 0;
        ret = __cpa_ioctl_getentry(cp, &args);
        if (ret != 0)
            continue;
        if (args.value != -1) {
            struct ldom_obj *ldom;
            char ldom_name[8];
            snprintf(ldom_name, 8, "ldom%d", i);

            ldom = kzalloc(sizeof(struct ldom_obj), GFP_KERNEL);
            ldom->cp = cp;
            ldom->ldomID = i;
            INIT_LIST_HEAD(&ldom->next);
            list_add_tail(&ldom->next, &cp->ldoms_list);

            // build ldomX kset
            ldom->kset = kset_create_and_add(ldom_name, NULL,
                                             &cp->kset_ldoms->kobj);
            if (!ldom->kset) {
                printk(KERN_ERR "cpa: unable to create %s kset\n", ldom_name);
                kfree(ldom);
                continue;
            }

            // add statistics
            ldom->stats.ldom_obj = ldom;	// back-pointer
            ldom->stats.kobj.kset = ldom->kset;
            ldom->stats.ktype.sysfs_ops = &ldom_sysfs_ops;
            ldom->stats.ktype.release = ldom_release;
            ldom->stats.ktype.default_attrs = extra->stats_attrs;
            ret = kobject_init_and_add(&ldom->stats.kobj, &ldom->stats.ktype, NULL,
                                       "statistics");
            if (ret) {
                printk(KERN_ERR "cpa: unable to create statistics for %s kset\n",
                       ldom_name);
                kobject_put(&ldom->stats.kobj);
                kset_unregister(ldom->kset);
                kfree(ldom);
                continue;
            }

            // add parameters
            ldom->params.ldom_obj = ldom;	// back-pointer
            ldom->params.kobj.kset = ldom->kset;
            ldom->params.ktype.sysfs_ops = &ldom_sysfs_ops;
            ldom->params.ktype.release = ldom_release;
            ldom->params.ktype.default_attrs = extra->params_attrs;
            ret = kobject_init_and_add(&ldom->params.kobj, &ldom->params.ktype, NULL,
                                       "parameters");
            if (ret) {
                printk(KERN_ERR "cpa: unable to create parameters for %s kset\n",
                       ldom_name);
                kobject_put(&ldom->params.kobj);
                kobject_put(&ldom->stats.kobj);
                kset_unregister(ldom->kset);
                kfree(ldom);
                continue;
            }

            // add triggers
            ldom->triggers.ldom_obj = ldom;	// back-pointer
            ldom->triggers.kobj.kset = ldom->kset;
            ldom->triggers.ktype.sysfs_ops = &ldom_sysfs_ops;
            ldom->triggers.ktype.release = ldom_release;
            ldom->triggers.ktype.default_attrs = extra->triggers_attrs;
            ret = kobject_init_and_add(&ldom->triggers.kobj, &ldom->triggers.ktype,
                                       NULL, "triggers");
            if (ret) {
                printk(KERN_ERR "cpa: unable to create triggers for %s kset\n",
                       ldom_name);
                kobject_put(&ldom->triggers.kobj);
                kobject_put(&ldom->stats.kobj);
                kset_unregister(ldom->kset);
                kfree(ldom);
                continue;
            }

            kobject_uevent(&ldom->stats.kobj,    KOBJ_ADD);
            kobject_uevent(&ldom->params.kobj,   KOBJ_ADD);
            kobject_uevent(&ldom->triggers.kobj, KOBJ_ADD);

            printk("ldom#%d ", ldom->ldomID);
        }
    }
    printk("\n");
}

void generic_cp_remove(struct control_plane *cp)
{
    struct ldom_obj *ldom;
    struct list_head *tmp, *pos;

    printk(KERN_INFO "generic_cp_remove called with cp: %s\n", cp->ident);
    spin_lock(&cp->lock);
    list_for_each_safe(pos, tmp, &cp->ldoms_list) {
        ldom = list_entry(pos, struct ldom_obj, next);
        list_del(pos);

        kobject_put(&ldom->stats.kobj);
        kobject_put(&ldom->params.kobj);
        kobject_put(&ldom->triggers.kobj);

        kset_unregister(ldom->kset);
        kfree(ldom);
    }
    spin_unlock(&cp->lock);
}
