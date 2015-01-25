#ifndef __PRM_GENERAL_CONTROLPLANE_HH__
#define __PRM_GENERAL_CONTROLPLANE_HH__

#include "params/GeneralControlPlane.hh"
#include "prm/ControlPlane.hh"

class GeneralControlPlane : public ControlPlane
{
  public:
    typedef GeneralControlPlaneParams Params;
    GeneralControlPlane(Params *p);
    virtual ~GeneralControlPlane();

  protected:
    const Params * param() const
    { return dynamic_cast<const Params *>(_params); }
};

#endif	// __PRM_GENERAL_CONTROLPLANE_HH__
