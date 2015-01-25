#include "cpa.h"

struct cp_attribute {
    struct attribute attr;
    ssize_t  (*show)(struct control_plane *cp, struct cp_attribute *attr, char *buf);
    ssize_t (*store)(struct control_plane *cp, struct cp_attribute *attr, const char *buf, size_t count);
};
#define to_cp_attr(x) container_of(x, struct cp_attribute, attr)

static ssize_t cp_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct cp_attribute *attribute;
    struct control_plane *cp;

    attribute = to_cp_attr(attr);
    cp = kobj_to_cp(kobj);

    if (!attribute->show)
        return -EIO;
    return attribute->show(cp, attribute, buf);
}

static ssize_t
cp_attr_store(struct kobject *kobj,
              struct attribute *attr,
              const char *buf, size_t len)
{
    struct cp_attribute *attribute;
    struct control_plane *cp;

    attribute = to_cp_attr(attr);
    cp = kobj_to_cp(kobj);

    if (!attribute->store)
        return -EIO;

    return attribute->store(cp, attribute, buf, len);
}

static struct sysfs_ops cp_sysfs_ops = {
    .show = cp_attr_show,
    .store = cp_attr_store,
};



static ssize_t
__cp_basic_attr_show(struct control_plane *cp, struct cp_attribute *attr, char *buf)
{
    if (strcmp(attr->attr.name, "ident") == 0)
        return sprintf(buf, "%s\n", cp->ident);
    else if (strcmp(attr->attr.name, "cpid") == 0)
        return sprintf(buf, "%d\n", cp->cpid);
    else if (strcmp(attr->attr.name, "type") == 0)
        return sprintf(buf, "%d\n", cp->type);
    else
        return sprintf(buf, "N/A\n");
}

#define __CP_ATTR_RO(_name) { \
    .attr = { .name = __stringify(_name), .mode = 0444 }, \
    .show = __cp_basic_attr_show,                         \
}
static struct cp_attribute ident_attribute = __CP_ATTR_RO(ident);
static struct cp_attribute cpid_attribute  = __CP_ATTR_RO(cpid);
static struct cp_attribute type_attribute  = __CP_ATTR_RO(type);

static struct attribute *cp_default_attrs[] = {
    &ident_attribute.attr,
    &cpid_attribute.attr,
    &type_attribute.attr,
    NULL,	/* NULL terminated list of attriutes */
};

struct kobj_type cp_ktype = {
    .sysfs_ops = &cp_sysfs_ops,
    .default_attrs = cp_default_attrs,
};
