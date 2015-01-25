#ifndef __MEM_PARD_MEMORYCTRL_HH__
#define __MEM_PARD_MEMORYCTRL_HH__

#include "mem/abstract_mem.hh"
#include "params/PARDMemoryCtrl.hh"

class PARDMemoryCtrl : public MemObject
{
  private:

    class RequestState : public Packet::SenderState
    {
      public:
        const PortID origSrc;
        const Addr origAddr;
        RequestState(PortID orig_src, Addr orig_addr)
            : origSrc(orig_src), origAddr(orig_addr)
        { }
    };

    class MemoryPort : public SlavePort
    {
        PARDMemoryCtrl& memory;
        MasterPort& internalPort;

      public:

        MemoryPort(const std::string& name, PARDMemoryCtrl& _memory)
            : SlavePort(name, &_memory), memory(_memory),
              internalPort(_memory.internal_port)
        { }

      protected:

        virtual Tick recvAtomic(PacketPtr pkt)
        { return memory.recvAtomic(pkt); }

        virtual void recvFunctional(PacketPtr pkt)
        {
            pkt->pushLabel(memory.name());
            memory.recvFunctional(pkt);
            pkt->popLabel();
        }

        virtual bool recvTimingReq(PacketPtr pkt)
        { return memory.recvTimingReq(pkt); }

        virtual AddrRangeList getAddrRanges() const
        { return memory.getAddrRanges(); }

        virtual void recvRetry()
        { internalPort.sendRetry(); }
    };

    class InternalPort : public MasterPort
    {
      private:
        PARDMemoryCtrl& memory;
        SlavePort& memoryPort;

      public:
        InternalPort(const std::string &name, PARDMemoryCtrl &_memory)
            : MasterPort(name, &_memory), memory(_memory),
              memoryPort(_memory.port)
        { }

      protected:
        virtual bool recvTimingResp(PacketPtr pkt)
        { return memory.recvTimingResp(pkt); }

        virtual void recvRetry()
        { return memoryPort.sendRetry(); }
    };

    MemoryPort port;
    InternalPort internal_port;
    std::vector<AbstractMemory *> memories;

  public:

    PARDMemoryCtrl(const PARDMemoryCtrlParams* p);

    virtual void init();

    virtual BaseSlavePort&
    getSlavePort(const std::string& if_name, PortID idx = InvalidPortID)
    {
        if (if_name == "port")
            return port;
        else
            return MemObject::getSlavePort(if_name, idx);
    }

    virtual BaseMasterPort&
    getMasterPort(const std::string& if_name, PortID idx = InvalidPortID)
    {
        if (if_name == "internal_port")
            return internal_port;
        else
            return MemObject::getMasterPort(if_name, idx);
    }

  protected:

    AddrRangeList getAddrRanges() const;

    Tick recvAtomic(PacketPtr pkt);
    void recvFunctional(PacketPtr pkt);
    bool recvTimingReq(PacketPtr pkt);
    bool recvTimingResp(PacketPtr pkt);

    virtual Addr remapAddr(uint16_t DSid, Addr addr) const;
   
};

#endif	// __MEM_PARD_MEMORYCTRL_HH__
