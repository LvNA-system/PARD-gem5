#include "debug/PARDMemoryCtrl.hh"
#include "mem/pard_mem_ctrl.hh"

PARDMemoryCtrl::PARDMemoryCtrl(const PARDMemoryCtrlParams* p)
    : MemObject(p),
      port(name() + ".port", *this),
      internal_port(name() + ".internal_port", *this)
      //memories(p->memories)
{
      memories.push_back(p->memories);
}

void
PARDMemoryCtrl::init()
{
    MemObject::init();

    // check port connection
    if (!internal_port.isConnected()) {
        fatal("PARD memory controller %s internal_port is unconnected!\n", name());
    }
    if (!port.isConnected()) {
        fatal("PARD memory controller %s is unconnected!\n", name());
    } else {
        port.sendRangeChange();
    }
}

AddrRangeList
PARDMemoryCtrl::getAddrRanges() const
{
    AddrRangeList ranges;
    for (auto mem : memories)
        ranges.push_back(mem->getAddrRange());
    return ranges;
}

Addr
PARDMemoryCtrl::remapAddr(uint16_t DSid, Addr addr) const
{
    if (DSid < 4) {
        DPRINTF(PARDMemoryCtrl, "[%d] 0x%016x ==> 0x%016x\n", DSid, addr, addr + (uint64_t)DSid*0x80000000);
        return addr + (uint64_t)DSid*0x80000000;
    }
    else {
        panic("PARDMemoryCtrl::remapAddr(): unknown DSid 0x%x\n", DSid);
        return addr;
    }
}

Tick
PARDMemoryCtrl::recvAtomic(PacketPtr pkt)
{
    Addr orig_addr = pkt->getAddr();
    pkt->setAddr(remapAddr(pkt->getDSid(), orig_addr));
    pkt->firstWordDelay = pkt->lastWordDelay = 0;
    Tick ret_tick = internal_port.sendAtomic(pkt);
    pkt->setAddr(orig_addr);
    return ret_tick;
}

bool
PARDMemoryCtrl::recvTimingReq(PacketPtr pkt)
{
    Addr orig_addr = pkt->getAddr();
    bool needsResponse = pkt->needsResponse();
    bool memInhibitAsserted = pkt->memInhibitAsserted();

    if (!memInhibitAsserted && needsResponse)
        pkt->pushSenderState(new RequestState(pkt->getSrc(), orig_addr));
    pkt->firstWordDelay = pkt->lastWordDelay = 0;

    pkt->setAddr(remapAddr(pkt->getDSid(), orig_addr));

    // Attempt to send the packet (always succeeds for inhibited
    // packets)
    bool successful = internal_port.sendTimingReq(pkt);

    // If not successful, restore the sender state
    if (!successful && needsResponse)
        delete pkt->popSenderState();

    return successful;
}

void
PARDMemoryCtrl::recvFunctional(PacketPtr pkt)
{
    Addr orig_addr = pkt->getAddr();
    pkt->setAddr(remapAddr(pkt->getDSid(), orig_addr));
    pkt->firstWordDelay = pkt->lastWordDelay = 0;
    internal_port.sendFunctional(pkt);
    pkt->setAddr(orig_addr);
}

bool
PARDMemoryCtrl::recvTimingResp(PacketPtr pkt)
{
    RequestState *req_state =
        dynamic_cast<RequestState*>(pkt->senderState);

    // panic if failed to restore initial sender state
    panic_if(!req_state,
             "PARDMemoryCtrl %s got a response without sender state.",
             name());
    pkt->popSenderState();

    PortID dest = pkt->getDest();
    Addr remapped_addr = pkt->getAddr();

    pkt->setDest(req_state->origSrc);
    pkt->setAddr(req_state->origAddr);
    pkt->firstWordDelay = pkt->lastWordDelay = 0;

    // Attemp to send the packet
    bool successful = port.sendTimingResp(pkt);

    // If packet successfully sent delete the sender state otherwise
    // restore state
    if (successful) {
        delete req_state;
    } else {
        // Don't delete anything and let the packet look like we did
        // not touch it
        pkt->pushSenderState(req_state);
        pkt->setDest(dest);
        pkt->setAddr(remapped_addr);
    }

    return successful;
}

PARDMemoryCtrl*
PARDMemoryCtrlParams::create()
{
    return new PARDMemoryCtrl(this);
}

