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

#define ADDRTYPE_MASK		0xC0000000
#define ADDRTYPE_CFGTBL		0x00000000
#define ADDRTYPE_CFGMEM		0x40000000
#define ADDRTYPE_SYSINFO	0x80000000

#define CFGTBL_TYPE_PARAM	0b00
#define CFGTBL_TYPE_STAT	0b01
#define CFGTBL_TYPE_TRIGGER	0b10

#define cfgtbl_addr2type(addr)		((addr & 0x30000000)>>28)
#define cfgtbl_addr2row(addr)		((addr & 0x0003FC00)>>10)
#define cfgtbl_addr2offset(addr)	(addr & 0x000003FF)
#define cfgmem_addr2offset(addr)	(addr & 0x00FFFFFF)
#define sysinfo_addr2offset(addr)	(addr & 0x000000FF)

#endif	// __PRM_CONTROLPLANE_HH__
