#ifndef __CPA_IOCTL_H__
#define __CPA_IOCTL_H__

#include <linux/ioctl.h>


#define CPA_IOC_MAGIC	'p'

typedef unsigned short LDomID;

struct cpa_ioctl_args_t {
    unsigned short ldom;
    unsigned long addr;
    unsigned long value;
};


/**
 * CPA ioctl opcode
 **/

#define CPA_IOCRESET	_IO(CPA_IOC_MAGIC, 0)

#define CPA_IOCSENTRY	_IOW(CPA_IOC_MAGIC, 1, struct cpa_ioctl_args_t)
#define CPA_IOCGENTRY	_IOR(CPA_IOC_MAGIC, 2, struct cpa_ioctl_args_t)

#define CPA_IOC_MAXNR	3


/**
 * CPA ioctl addr decoder
 **/

struct CPA_CACHE_IOCADDR {
    uint64_t waymask;
    uint64_t capacity;
    uint64_t hit_count;
    uint64_t miss_count;
};

// Statistics of memory controller 
struct CPA_MEMCNTRL_IOCADDR_STATS {
    uint64_t readReqs;
    uint64_t writeReqs;
    uint64_t readBursts;
    uint64_t writeBursts;
    uint64_t bytesReadDRAM;
    uint64_t bytesReadWrQ;
    uint64_t bytesWritten;
    uint64_t bytesReadSys;
    uint64_t bytesWrittenSys;
    uint64_t servicedByWrQ;
    uint64_t mergedWrBursts;
    uint64_t neitherReadNorWrite;
    uint64_t numRdRetry;
    uint64_t numWrRetry;
    uint64_t totGap;
    uint64_t rdQLen;
    uint64_t wrQLen;
    // Latencies summed over all requests
    uint64_t totQLat;
    uint64_t totMemAccLat;
    uint64_t totBusLat;
    // Average latencies per request
    uint64_t avgQLat;
    uint64_t avgBusLat;
    uint64_t avgMemAccLat;
    // Average bandwidth
    uint64_t avgRdBW;
    uint64_t avgWrBW;
    uint64_t avgRdBWSys;
    uint64_t avgWrBWSys;
    uint64_t peakBW;
    uint64_t busUtil;
    uint64_t busUtilRead;
    uint64_t busUtilWrite;
    // Row hit count and rate
    uint64_t readRowHits;
    uint64_t writeRowHits;
    uint64_t readRowHitRate;
    uint64_t writeRowHitRate;
    uint64_t avgGap;
    uint64_t pageHitRate;
};

// Parameters of memory controller
struct CPA_MEMCNTRL_IOCADDR_PARAMS {
    uint64_t priority;
    uint64_t effective_priority;
    uint64_t row_buffer_mask;
};

// Triggers of memory controller
struct CPA_MEMCNTRL_IOCADDR_TRIGGERS {
    uint64_t nouse;
};

#define __CPA_OFFSET_OF(type, field) (unsigned long)(&(((type *)0)->field))

#endif	// __CPA_IOCTL_H__
