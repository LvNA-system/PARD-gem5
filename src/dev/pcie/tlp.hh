#ifndef __DEV_PCIE_TLP_HH__
#define __DEV_PCIE_TLP_HH__

#include "mem/packet.hh"

#define MAKE_PCI_ID(bus, dev, fun)  \
    (((bus&0xFFFF)<<16) | ((dev&&0xFF)<<8) | (fun&&0xFF))

class PciExpressTLP : public Packet
{
  public:
    enum TLPType
    {
        MRd, MRdLk, MWr,
        IORd, IOWr,
        CfgRd0, CfgWr0, CfgRd1, CfgWr1,
        Msg, MsgD,
        NUM_TLPTYPES
    };

    const TLPType type;
    const uint32_t destID;
    const PacketPtr pkt;

  public:
    PciExpressTLP(TLPType _type, uint32_t _destID, const PacketPtr _pkt)
        : Packet(_pkt->req, _pkt->cmd),
          type(_type), destID(_destID), pkt(_pkt)
    {
        setAddr(destID);
    }
    
};

#endif // __DEV_PCIE_TLP_HH__
