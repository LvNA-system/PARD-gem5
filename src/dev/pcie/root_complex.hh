#ifndef __DEV_PCIE_ROOTCOMPLEX_HH__
#define __DEV_PCIE_ROOTCOMPLEX_HH__

#include <deque>

#include "base/types.hh"
#include "dev/pcie/tlp.hh"
#include "ext/xbridge.hh"
#include "mem/mem_object.hh"
#include "params/RootComplex.hh"


class RootComplex : public XBridge
{

  protected:

    class RCSlavePort : public BridgeSlavePort
    {
      private:
        RootComplex &rc;

      public:
        RCSlavePort(const std::string &_name, RootComplex &_rc,
                    BridgeMasterPort& _masterPort, Cycles _delay,
                    int _resp_limit, std::vector<AddrRange> _ranges)
            : BridgeSlavePort(_name, _rc, _masterPort, _delay, _resp_limit,
                              _ranges),
              rc(_rc)
        { }
    };

    class RCMasterPort : public BridgeMasterPort
    {
      private:
        RootComplex &rc;

      public:
        RCMasterPort(const std::string &_name, RootComplex &_rc,
                     BridgeSlavePort& _slavePort, Cycles _delay,
                     int _req_limit)
            : BridgeMasterPort(_name, rc, _slavePort, _delay, _req_limit),
              rc(_rc)
        { }

        virtual void schedTimingReq(PacketPtr pkt, Tick when);

      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual bool checkFunctional(PacketPtr pkt);

    };

    /** Slave port to receive CPU request */
    RCSlavePort slavePort;
    /** Master port to send PCI-E TLP request */
    RCMasterPort masterPort;

  protected:

    PciExpressTLP * buildTLP(PacketPtr pkt);

  public:

    virtual BaseMasterPort& getMasterPort(const std::string& if_name,
                                          PortID idx = InvalidPortID);
    virtual BaseSlavePort& getSlavePort(const std::string& if_name,
                                        PortID idx = InvalidPortID);

    virtual void init();

    typedef RootComplexParams Params;

    RootComplex(Params *p);

};

#endif  // __DEV_PCIE_ROOTCOMPLEX_HH__
