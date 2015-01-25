#include "arch/x86/x86_traits.hh"
#include "dev/pcie/root_complex.hh"
#include "debug/RootComplex.hh"
#include "params/RootComplex.hh"

RootComplex::RootComplex(Params *p)
    : XBridge(p),
      slavePort(p->name + ".slave", *this, masterPort,
                ticksToCycles(p->delay), p->resp_size, p->ranges),
      masterPort(p->name + ".master", *this, slavePort,
                 ticksToCycles(p->delay), p->req_size)
{
}

BaseMasterPort&
RootComplex::getMasterPort(const std::string &if_name, PortID idx)
{
    if (if_name == "master")
        return masterPort;
    else
        // pass it along to our super class
        return XBridge::getMasterPort(if_name, idx);
}

BaseSlavePort&
RootComplex::getSlavePort(const std::string &if_name, PortID idx)
{
    if (if_name == "slave")
        return slavePort;
    else
        // pass it along to our super class
        return XBridge::getSlavePort(if_name, idx);
}

void
RootComplex::init()
{
    // make sure both sides are connected and have the same block size
    if (!slavePort.isConnected() || !masterPort.isConnected())
        fatal("Both ports of a bridge must be connected.\n");

    // notify the master side  of our address ranges
    slavePort.sendRangeChange();
}

PciExpressTLP *
RootComplex::buildTLP(PacketPtr pkt)
{
    PciExpressTLP *tlp = NULL;

    /* >= 0xC000000000000000 */
    if (pkt->getAddr() >= X86ISA::PhysAddrPrefixPciConfig) {
        // PCI Config 
        tlp = new PciExpressTLP(PciExpressTLP::CfgRd1, 0, pkt);
    }
    /* >= 0xA000000000000000 */
    else if (pkt->getAddr() >= X86ISA::PhysAddrPrefixInterrupts) {
        // Interrupts? 
    }
    /* >= 0x8000000000000000 */
    else if (pkt->getAddr() >= X86ISA::PhysAddrPrefixIO) {
        // IO Access
    }
    /* >= 0x2000000000000000 */
    else if (pkt->getAddr() >= X86ISA::PhysAddrPrefixLocalAPIC) {
        // LocalAPIC?
    }
    /* Normal Memory */
    else {
        // Memory Access
    }
    return tlp;
}

void
RootComplex::RCMasterPort::schedTimingReq(PacketPtr pkt, Tick when)
{
    // If we expect to see a response, we need to restore the source
    // and destination field that is potentially changed by a second
    // crossbar
    if (!pkt->memInhibitAsserted() && pkt->needsResponse()) {
        // Update the sender state so we can deal with the response
        // appropriately
        pkt->pushSenderState(new RequestState(pkt->getSrc()));
    }

    // If we're about to put this packet at the head of the queue, we
    // need to schedule an event to do the transmit.  Otherwise there
    // should already be an event scheduled for sending the head
    // packet.
    if (transmitList.empty()) {
        rc.schedule(sendEvent, when);
    }

    assert(transmitList.size() != reqQueueLimit);

    // Build PCI-Express TLP packet and add it to transmitList
    PciExpressTLP *tlp = rc.buildTLP(pkt);
    transmitList.push_back(DeferredPacket(tlp, when));
}

bool
RootComplex::RCMasterPort::recvTimingResp(PacketPtr pkt)
{
    PciExpressTLP *tlp = dynamic_cast<PciExpressTLP *>(pkt);
    if (!tlp)
        panic("RootComplex::RCMasterPort receive non-TLP packet.");
    pkt = tlp->pkt;
    delete tlp;

    // all checks are done when the request is accepted on the slave
    // side, so we are guaranteed to have space for the response
    DPRINTF(RootComplex, "recvTimingResp: %s addr 0x%x\n",
            pkt->cmdString(), pkt->getAddr());

    // @todo: We need to pay for this and not just zero it out
    pkt->firstWordDelay = pkt->lastWordDelay = 0;

    slavePort.schedTimingResp(pkt, rc.clockEdge(delay));

    return true;
}

bool
RootComplex::RCMasterPort::checkFunctional(PacketPtr pkt)
{
    bool found = false;
    auto i = transmitList.begin();

    while(i != transmitList.end() && !found) {
         PciExpressTLP *tlp = dynamic_cast<PciExpressTLP *>((*i).pkt);
         assert(tlp);
         if (pkt->checkFunctional(tlp->pkt)) {
              pkt->makeResponse();
              found = true;
         }
         ++i;
    }

    return found;
}

RootComplex *
RootComplexParams::create()
{
    return new RootComplex(this);
}

