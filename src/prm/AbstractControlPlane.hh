#ifndef __PRM_ABSTRACT_CONTROLPLANE_HH__
#define __PRM_ABSTRACT_CONTROLPLANE_HH__

#include <string>

#include "sim/clocked_object.hh"
#include "params/AbstractControlPlane.hh"

class AbstractControlPlane : public ClockedObject
{
  public:
    virtual void updateStat(int16_t DSid, const std::string &stat,
                            uint32_t value)
    {
        checkTrigger(DSid, stat);
    }

    virtual void incrStat(int16_t DSid, const std::string &stat,
                          int32_t delta)
    {
        checkTrigger(DSid, stat);
    }

    virtual uint32_t queryParam(int16_t DSid, const std::string &param)
    {
        panic("un-impl AbstractControlPlane::queryParam()\n");
        return 0;
    }

    virtual void checkTrigger(int16_t DSid, const std::string& stat)
    {
    }

  public:
    typedef struct AbstractControlPlaneParams Params;
    AbstractControlPlane(const Params *params) : ClockedObject(params) {}
    virtual ~AbstractControlPlane() {}
};

#endif	// __PRM_CONTROLPLANE_HH__
