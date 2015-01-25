#include "ctrlplane.h"

/**
 * Builtin memcntrl ldom attributes in "/sys/cpa/cpaX/ldom/ldomX/______"
 **/

#define __MEMCNTRL_LDOM_ATTR(_base, _type, _name, _mode) \
    __GENERIC_LDOM_ATTR(_name, _base + __CPA_OFFSET_OF(_type, _name)/sizeof(uint64_t), _mode)

#define __MEMCNTRL_PARAMS_ATTR(_name, _mode) \
    __MEMCNTRL_LDOM_ATTR(0x0000,   struct CPA_MEMCNTRL_IOCADDR_PARAMS, _name, _mode)
#define __MEMCNTRL_STATS_ATTR(_name, _mode) \
    __MEMCNTRL_LDOM_ATTR(0x4000,    struct CPA_MEMCNTRL_IOCADDR_STATS, _name, _mode)
#define __MEMCNTRL_TRIGGERS_ATTR(_name, _mode) \
    __MEMCNTRL_LDOM_ATTR(0x8000, struct CPA_MEMCNTRL_IOCADDR_TRIGGERS, _name, _mode)

// Statistics attributes
static struct ldom_attribute __stats_attributes[] = {
    __MEMCNTRL_STATS_ATTR(           readReqs, 0444),
    __MEMCNTRL_STATS_ATTR(          writeReqs, 0444),
    __MEMCNTRL_STATS_ATTR(         readBursts, 0444),
    __MEMCNTRL_STATS_ATTR(        writeBursts, 0444),
    __MEMCNTRL_STATS_ATTR(      bytesReadDRAM, 0444),
    __MEMCNTRL_STATS_ATTR(       bytesReadWrQ, 0444),
    __MEMCNTRL_STATS_ATTR(       bytesWritten, 0444),
    __MEMCNTRL_STATS_ATTR(       bytesReadSys, 0444),
    __MEMCNTRL_STATS_ATTR(    bytesWrittenSys, 0444),
    __MEMCNTRL_STATS_ATTR(      servicedByWrQ, 0444),
    __MEMCNTRL_STATS_ATTR(     mergedWrBursts, 0444),
    __MEMCNTRL_STATS_ATTR(neitherReadNorWrite, 0444),
    __MEMCNTRL_STATS_ATTR(         numRdRetry, 0444),
    __MEMCNTRL_STATS_ATTR(         numWrRetry, 0444),
    __MEMCNTRL_STATS_ATTR(             totGap, 0444),
    __MEMCNTRL_STATS_ATTR(             rdQLen, 0444),
    __MEMCNTRL_STATS_ATTR(             wrQLen, 0444),
    // Latencies summed over all requests
    __MEMCNTRL_STATS_ATTR(     totQLat, 0444),
    __MEMCNTRL_STATS_ATTR(totMemAccLat, 0444),
    __MEMCNTRL_STATS_ATTR(   totBusLat, 0444),
    // Average latencies per request
    __MEMCNTRL_STATS_ATTR(     avgQLat, 0444),
    __MEMCNTRL_STATS_ATTR(   avgBusLat, 0444),
    __MEMCNTRL_STATS_ATTR(avgMemAccLat, 0444),
        // Average bandwidth
    __MEMCNTRL_STATS_ATTR(     avgRdBW, 0444),
    __MEMCNTRL_STATS_ATTR(     avgWrBW, 0444),
    __MEMCNTRL_STATS_ATTR(  avgRdBWSys, 0444),
    __MEMCNTRL_STATS_ATTR(  avgWrBWSys, 0444),
    __MEMCNTRL_STATS_ATTR(      peakBW, 0444),
    __MEMCNTRL_STATS_ATTR(     busUtil, 0444),
    __MEMCNTRL_STATS_ATTR( busUtilRead, 0444),
    __MEMCNTRL_STATS_ATTR(busUtilWrite, 0444),
    // Row hit count and rate
    __MEMCNTRL_STATS_ATTR(    readRowHits, 0444),
    __MEMCNTRL_STATS_ATTR(   writeRowHits, 0444),
    __MEMCNTRL_STATS_ATTR( readRowHitRate, 0444),
    __MEMCNTRL_STATS_ATTR(writeRowHitRate, 0444),
    __MEMCNTRL_STATS_ATTR(         avgGap, 0444),
    __MEMCNTRL_STATS_ATTR(    pageHitRate, 0444),
};

static struct attribute *stats_attrs[] = {
    &__stats_attributes[0].attr,  &__stats_attributes[1].attr,
    &__stats_attributes[2].attr,  &__stats_attributes[3].attr,
    &__stats_attributes[4].attr,  &__stats_attributes[5].attr,
    &__stats_attributes[6].attr,  &__stats_attributes[7].attr,
    &__stats_attributes[8].attr,  &__stats_attributes[9].attr,
    &__stats_attributes[10].attr, &__stats_attributes[11].attr,
    &__stats_attributes[12].attr, &__stats_attributes[13].attr,
    &__stats_attributes[14].attr, &__stats_attributes[15].attr,
    &__stats_attributes[16].attr, &__stats_attributes[17].attr,
    &__stats_attributes[18].attr, &__stats_attributes[19].attr,
    &__stats_attributes[20].attr, &__stats_attributes[21].attr,
    &__stats_attributes[22].attr, &__stats_attributes[23].attr,
    &__stats_attributes[24].attr, &__stats_attributes[25].attr,
    &__stats_attributes[26].attr,
    NULL,
};

// Parameters attributes
static struct ldom_attribute __params_attributes[] = {
    __MEMCNTRL_PARAMS_ATTR(priority, 0666),
    __MEMCNTRL_PARAMS_ATTR(effective_priority, 0666),
    __MEMCNTRL_PARAMS_ATTR(row_buffer_mask, 0666),
};

static struct attribute *params_attrs[] = {
    &__params_attributes[0].attr, &__params_attributes[1].attr,
    &__params_attributes[2].attr,
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
struct control_plane_op memcntrl_cp_op =
    GENERIC_CONTROL_PLANE_OP("MEMCNTRL_CP", 'M', &extras);

