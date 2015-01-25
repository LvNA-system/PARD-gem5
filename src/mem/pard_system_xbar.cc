/*
 * Copyright (c) 2014 Institute of Computing Technology, CAS
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Jiuyue Ma
 */

/**
 * @file
 * Definition of coherent PARD system crossbar.
 */

#include "base/misc.hh"
#include "base/trace.hh"
#include "debug/AddrRanges.hh"
#include "debug/PARDSystemXBar.hh"
#include "mem/pard_system_xbar.hh"
#include "sim/system.hh"

PARDSystemXBar::PARDSystemXBar(const PARDSystemXBarParams *p,
                         const unsigned int port_slave_connection_count)
    : CoherentXBar(p),
      memoryPort(NULL),
      memoryPortID(InvalidPortID),
      memoryRanges(p->memory_ranges),
      ioPort(NULL),
      ioPortID(InvalidPortID),
      ioRanges(p->io_ranges)
{
    // create the slave ports, because they are faked in CoherentXBar
    // see PARDSystemXBarParams::create()
    for (int i = 0; i < port_slave_connection_count; ++i) {
        std::string portName = csprintf("%s.slave[%d]", name(), i);
        SlavePort* bp = new PARDSystemXBarSlavePort(portName, *this, i);
        slavePorts.push_back(bp);
        respLayers.push_back(new RespLayer(*bp, *this,
                                           csprintf(".respLayer%d", i)));
        snoopRespPorts.push_back(new SnoopRespPort(*bp, *this));
    }

    // create memory port
    if (p->port_memory_port_connection_count) {
        memoryPortID = masterPorts.size();
        std::string portName = csprintf("%s.memory_port", name());
        memoryPort = new PARDSystemXBarMasterPort(portName, *this,
                                                 memoryPortID);
        masterPorts.push_back(memoryPort);
        reqLayers.push_back(new ReqLayer(*memoryPort, *this,
                                             csprintf(".reqLayer%d",
                                                      memoryPortID)));
        snoopLayers.push_back(new SnoopLayer(*memoryPort, *this,
                                             csprintf(".snoopLayer%d",
                                                      memoryPortID)));

        // add memory range to fixedPortMap
        for (const auto& r: memoryRanges) {
            if (fixedPortMap.insert(r, memoryPortID) == fixedPortMap.end()) {
                PortID conflict_id = fixedPortMap.find(r)->second;
                fatal("%s has two ports responding within range %s:\n"
                      "\t%s\n\t%s\n",
                      name(),
                      r.to_string(),
                      masterPorts[memoryPortID]->name(),
                      masterPorts[conflict_id]->name());
            }
        }
    }

    // create I/O port
    if (p->port_io_port_connection_count) {
        ioPortID = masterPorts.size();
        std::string portName = csprintf("%s.io_port", name());
        ioPort = new PARDSystemXBarMasterPort(portName, *this,
                                                      ioPortID);
        masterPorts.push_back(ioPort);
        reqLayers.push_back(new ReqLayer(*ioPort, *this,
                                             csprintf(".reqLayer%d",
                                                      ioPortID)));
        snoopLayers.push_back(new SnoopLayer(*ioPort, *this,
                                             csprintf(".snoopLayer%d",
                                                      ioPortID)));
        // add io range to fixedPortMap
        for (const auto& r: ioRanges) {
            if (fixedPortMap.insert(r, ioPortID) == fixedPortMap.end()) {
                PortID conflict_id = fixedPortMap.find(r)->second;
                fatal("%s has two ports responding within range %s:\n"
                      "\t%s\n\t%s\n",
                      name(),
                      r.to_string(),
                      masterPorts[ioPortID]->name(),
                      masterPorts[conflict_id]->name());
            }
        }
    }
}

PARDSystemXBar::~PARDSystemXBar()
{
}

AddrRangeList
PARDSystemXBar::getAddrRanges() const
{
    AddrRangeList ranges = CoherentXBar::getAddrRanges();
    for (auto r : memoryRanges)
        ranges.push_back(r);
    for (auto r : ioRanges)
        ranges.push_back(r);
    return ranges;
}

BaseMasterPort &
PARDSystemXBar::getMasterPort(const std::string &if_name, PortID idx)
{
    if (if_name == "memory_port") {
        return *memoryPort;
    } else if (if_name == "io_port") {
        return *ioPort;
    } else {
        return CoherentXBar::getMasterPort(if_name, idx);
    }
}

bool
PARDSystemXBar::recvTimingReq(PacketPtr pkt, PortID slave_port_id)
{
    // determine the source port based on the id
    SlavePort *src_port = slavePorts[slave_port_id];

    // remember if the packet is an express snoop
    bool is_express_snoop = pkt->isExpressSnoop();

    // determine the destination based on the address
    // check memory and io port first, then all master port
    PortID master_port_id = InvalidPortID;
    auto i = fixedPortMap.find(pkt->getAddr());
    if (i != fixedPortMap.end())
        master_port_id = i->second;
    else
        master_port_id = findPort(pkt->getAddr());

    // test if the crossbar should be considered occupied for the current
    // port, and exclude express snoops from the check
    if (!is_express_snoop && !reqLayers[master_port_id]->tryTiming(src_port)) {
        DPRINTF(PARDSystemXBar, "recvTimingReq: src %s %s 0x%x BUSY\n",
                src_port->name(), pkt->cmdString(), pkt->getAddr());
        return false;
    }

    DPRINTF(PARDSystemXBar, "recvTimingReq: src %s %s expr %d 0x%x\n",
            src_port->name(), pkt->cmdString(), is_express_snoop,
            pkt->getAddr());

    // store size and command as they might be modified when
    // forwarding the packet
    unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
    unsigned int pkt_cmd = pkt->cmdToIndex();

    // set the source port for routing of the response
    pkt->setSrc(slave_port_id);

    calcPacketTiming(pkt);
    Tick packetFinishTime = pkt->lastWordDelay + curTick();

    // uncacheable requests need never be snooped
    if (!pkt->req->isUncacheable() && !system->bypassCaches()) {
        // the packet is a memory-mapped request and should be
        // broadcasted to our snoopers but the source
        if (snoopFilter) {
            // check with the snoop filter where to forward this packet
            auto sf_res = snoopFilter->lookupRequest(pkt, *src_port);
            packetFinishTime += sf_res.second * clockPeriod();
            DPRINTF(PARDSystemXBar, "recvTimingReq: src %s %s 0x%x"\
                    " SF size: %i lat: %i\n", src_port->name(),
                    pkt->cmdString(), pkt->getAddr(), sf_res.first.size(),
                    sf_res.second);
            forwardTiming(pkt, slave_port_id, sf_res.first);
        } else {
            forwardTiming(pkt, slave_port_id);
        }
    }

    // remember if we add an outstanding req so we can undo it if
    // necessary, if the packet needs a response, we should add it
    // as outstanding and express snoops never fail so there is
    // not need to worry about them
    bool add_outstanding = !is_express_snoop && pkt->needsResponse();

    // keep track that we have an outstanding request packet
    // matching this request, this is used by the coherency
    // mechanism in determining what to do with snoop responses
    // (in recvTimingSnoop)
    if (add_outstanding) {
        // we should never have an exsiting request outstanding
        assert(outstandingReq.find(pkt->req) == outstandingReq.end());
        outstandingReq.insert(pkt->req);
    }

    // Note: Cannot create a copy of the full packet, here.
    MemCmd orig_cmd(pkt->cmd);

    // since it is a normal request, attempt to send the packet
    bool success = masterPorts[master_port_id]->sendTimingReq(pkt);

    if (snoopFilter && !pkt->req->isUncacheable()
        && !system->bypassCaches()) {
        // The packet may already be overwritten by the sendTimingReq function.
        // The snoop filter needs to see the original request *and* the return
        // status of the send operation, so we need to recreate the original
        // request.  Atomic mode does not have the issue, as there the send
        // operation and the response happen instantaneously and don't need two
        // phase tracking.
        MemCmd tmp_cmd(pkt->cmd);
        pkt->cmd = orig_cmd;
        // Let the snoop filter know about the success of the send operation
        snoopFilter->updateRequest(pkt, *src_port, !success);
        pkt->cmd = tmp_cmd;
    }

    // if this is an express snoop, we are done at this point
    if (is_express_snoop) {
        assert(success);
        snoops++;
    } else {
        // for normal requests, check if successful
        if (!success)  {
            // inhibited packets should never be forced to retry
            assert(!pkt->memInhibitAsserted());

            // if it was added as outstanding and the send failed, then
            // erase it again
            if (add_outstanding)
                outstandingReq.erase(pkt->req);

            // undo the calculation so we can check for 0 again
            pkt->firstWordDelay = pkt->lastWordDelay = 0;

            DPRINTF(PARDSystemXBar, "recvTimingReq: src %s %s 0x%x RETRY\n",
                    src_port->name(), pkt->cmdString(), pkt->getAddr());

            // update the layer state and schedule an idle event
            reqLayers[master_port_id]->failedTiming(src_port,
                                                    clockEdge(headerCycles));
        } else {
            // update the layer state and schedule an idle event
            reqLayers[master_port_id]->succeededTiming(packetFinishTime);
        }
    }

    // stats updates only consider packets that were successfully sent
    if (success) {
        pktCount[slave_port_id][master_port_id]++;
        pktSize[slave_port_id][master_port_id] += pkt_size;
        transDist[pkt_cmd]++;
    }

    return success;
}

void
PARDSystemXBar::recvTimingSnoopReq(PacketPtr pkt, PortID master_port_id)
{
    DPRINTF(PARDSystemXBar, "recvTimingSnoopReq: src %s %s 0x%x\n",
            masterPorts[master_port_id]->name(), pkt->cmdString(),
            pkt->getAddr());

    // update stats here as we know the forwarding will succeed
    transDist[pkt->cmdToIndex()]++;
    snoops++;

    // we should only see express snoops from caches
    assert(pkt->isExpressSnoop());

    // set the source port for routing of the response
    pkt->setSrc(master_port_id);

    if (snoopFilter) {
        // let the Snoop Filter work its magic and guide probing
        auto sf_res = snoopFilter->lookupSnoop(pkt);
        // No timing here: packetFinishTime += sf_res.second * clockPeriod();
        DPRINTF(PARDSystemXBar, "recvTimingSnoopReq: src %s %s 0x%x"\
                " SF size: %i lat: %i\n", masterPorts[master_port_id]->name(),
                pkt->cmdString(), pkt->getAddr(), sf_res.first.size(),
                sf_res.second);

        // forward to all snoopers
        forwardTiming(pkt, InvalidPortID, sf_res.first);
    } else {
        forwardTiming(pkt, InvalidPortID);
    }

    // a snoop request came from a connected slave device (one of
    // our master ports), and if it is not coming from the slave
    // device responsible for the address range something is
    // wrong, hence there is nothing further to do as the packet
    // would be going back to where it came from
    assert(((fixedPortMap.find(pkt->getAddr()) != fixedPortMap.end())
              && (fixedPortMap.find(pkt->getAddr())->second == master_port_id)) || 
           master_port_id == findPort(pkt->getAddr()));
}

Tick
PARDSystemXBar::recvAtomic(PacketPtr pkt, PortID slave_port_id)
{
    DPRINTF(PARDSystemXBar, "recvAtomic: packet src %s addr 0x%x cmd %s\n",
            slavePorts[slave_port_id]->name(), pkt->getAddr(),
            pkt->cmdString());

    unsigned int pkt_size = pkt->hasData() ? pkt->getSize() : 0;
    unsigned int pkt_cmd = pkt->cmdToIndex();

    MemCmd snoop_response_cmd = MemCmd::InvalidCmd;
    Tick snoop_response_latency = 0;

    // uncacheable requests need never be snooped
    if (!pkt->req->isUncacheable() && !system->bypassCaches()) {
        // forward to all snoopers but the source
        std::pair<MemCmd, Tick> snoop_result;
        if (snoopFilter) {
            // check with the snoop filter where to forward this packet
            auto sf_res =
                snoopFilter->lookupRequest(pkt, *slavePorts[slave_port_id]);
            snoop_response_latency += sf_res.second * clockPeriod();
            DPRINTF(PARDSystemXBar, "%s: src %s %s 0x%x"\
                    " SF size: %i lat: %i\n", __func__,
                    slavePorts[slave_port_id]->name(), pkt->cmdString(),
                    pkt->getAddr(), sf_res.first.size(), sf_res.second);
            snoop_result = forwardAtomic(pkt, slave_port_id, InvalidPortID,
                                         sf_res.first);
        } else {
            snoop_result = forwardAtomic(pkt, slave_port_id);
        }
        snoop_response_cmd = snoop_result.first;
        snoop_response_latency += snoop_result.second;
    }

    // even if we had a snoop response, we must continue and also
    // perform the actual request at the destination.
    // check memory and io port first, then all master port
    PortID master_port_id = InvalidPortID;
    auto i = fixedPortMap.find(pkt->getAddr());
    if (i != fixedPortMap.end())
        master_port_id = i->second;
    else
        master_port_id = findPort(pkt->getAddr());

    // stats updates for the request
    pktCount[slave_port_id][master_port_id]++;
    pktSize[slave_port_id][master_port_id] += pkt_size;
    transDist[pkt_cmd]++;

    // forward the request to the appropriate destination
    Tick response_latency = masterPorts[master_port_id]->sendAtomic(pkt);

    // Lower levels have replied, tell the snoop filter
    if (snoopFilter && !pkt->req->isUncacheable() && !system->bypassCaches() &&
        pkt->isResponse()) {
        snoopFilter->updateResponse(pkt, *slavePorts[slave_port_id]);
    }

    // if we got a response from a snooper, restore it here
    if (snoop_response_cmd != MemCmd::InvalidCmd) {
        // no one else should have responded
        assert(!pkt->isResponse());
        pkt->cmd = snoop_response_cmd;
        response_latency = snoop_response_latency;
    }

    // add the response data
    if (pkt->isResponse()) {
        pkt_size = pkt->hasData() ? pkt->getSize() : 0;
        pkt_cmd = pkt->cmdToIndex();

        // stats updates
        pktCount[slave_port_id][master_port_id]++;
        pktSize[slave_port_id][master_port_id] += pkt_size;
        transDist[pkt_cmd]++;
    }

    // @todo: Not setting first-word time
    pkt->lastWordDelay = response_latency;
    return response_latency;
}

void
PARDSystemXBar::recvFunctional(PacketPtr pkt, PortID slave_port_id)
{
    if (!pkt->isPrint()) {
        // don't do DPRINTFs on PrintReq as it clutters up the output
        DPRINTF(PARDSystemXBar,
                "recvFunctional: packet src %s addr 0x%x cmd %s\n",
                slavePorts[slave_port_id]->name(), pkt->getAddr(),
                pkt->cmdString());
    }

    // uncacheable requests need never be snooped
    if (!pkt->req->isUncacheable() && !system->bypassCaches()) {
        // forward to all snoopers but the source
        forwardFunctional(pkt, slave_port_id);
    }

    // there is no need to continue if the snooping has found what we
    // were looking for and the packet is already a response
    if (!pkt->isResponse()) {
        // check memory and io port first, then all master port
        PortID dest_id = InvalidPortID;
        auto i = fixedPortMap.find(pkt->getAddr());
        if (i != fixedPortMap.end())
            dest_id = i->second;
        else
            dest_id = findPort(pkt->getAddr());

        masterPorts[dest_id]->sendFunctional(pkt);
    }
}

PARDSystemXBar *
PARDSystemXBarParams::create()
{
    // Fake slave port to CoherentXBar
    unsigned int fake_count = this->port_slave_connection_count;
    this->port_slave_connection_count=0;
    return new PARDSystemXBar(this, fake_count);
}

