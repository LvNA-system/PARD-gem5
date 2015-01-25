#ifndef __PRM_CONTROLPLANE_HH__
#define __PRM_CONTROLPLANE_HH__

#include "params/ControlPlane.hh"
#include "prm/AbstractControlPlane.hh"
#include "prm/interfaces.hh"

/**
 * Forward declare of CPConnector class.
 */
class CPConnector;

/**
 * ControlPlane connect to CPN use CPConnector, and define
 * interfaces used by CPConnector.
 */
class ControlPlane : public AbstractControlPlane
{
  protected:
    CPConnector *connector;

  public:
    typedef ControlPlaneParams Params;
    ControlPlane(const Params *p);
    virtual ~ControlPlane();

  public:
    void registerCommandHandler(ICommandHandler *handler);

    virtual uint64_t queryTable(uint16_t DSid, uint32_t addr) { return 0; }
    virtual void updateTable(uint16_t DSid, uint32_t addr, uint64_t data) {}

  protected:
    const Params * params() const
    { return dynamic_cast<const Params *>(_params); }
};

#endif	// __PRM_CONTROLPLANE_HH__
