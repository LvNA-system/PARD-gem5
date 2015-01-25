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
#include "debug/TagXBar.hh"
#include "mem/tag_xbar.hh"
#include "sim/system.hh"
#include "debug/CoherentXBar.hh"

TagXBar::TagXBar(const TagXBarParams *p)
    : MemObject(p),
      DSid(p->DSid), DSid_base_addr(p->DSid_base_addr),
      xbar(p),
      slavePort(csprintf("%s.internal_slave", name()),
                *this, InvalidPortID, masterPort),
      masterPort(csprintf("%s.internal_master", name()),
                *this, InvalidPortID, slavePort)
{
    xbar.getMasterPort("master", 0).bind(slavePort);
}

TagXBar::~TagXBar()
{
}

bool
TagXBar::recvTimingReq(PacketPtr pkt)
{
    DPRINTF(CoherentXBar, "TagXBar::recvTimingReq: %s expr %d 0x%x\n",
            pkt->cmdString(), pkt->isExpressSnoop(), pkt->getAddr());

    bool needsResponse = pkt->needsResponse();
    bool memInhibitAsserted = pkt->memInhibitAsserted();

    if (!memInhibitAsserted && needsResponse)
        pkt->pushSenderState(new RequestState(pkt->getSrc()));

    // Attempt to send the packet (always succeeds for inhibited
    // packets)
    bool successful = masterPort.sendTimingReq(pkt);

    // If not successful, restore the sender state
    if (!successful && needsResponse)
        delete pkt->popSenderState();

    return successful;
}

bool
TagXBar::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(CoherentXBar, "TagXBar::recvTimingResp: %s 0x%x\n",
            pkt->cmdString(), pkt->getAddr());
    RequestState *req_state =
        dynamic_cast<RequestState*>(pkt->popSenderState());
    // panic if failed to restore initial sender state
    panic_if(!req_state,
             "TagXBar %s got a response without sender state.",
             name());

    PortID dest = pkt->getDest();

    pkt->setDest(req_state->origSrc);
    pkt->firstWordDelay = pkt->lastWordDelay = 0;

    // Attemp to send the packet
    bool successful = slavePort.sendTimingResp(pkt);

    // If packet successfully sent delete the sender state otherwise
    // restore state
    if (successful) {
        delete req_state;
    } else {
        // Don't delete anything and let the packet look like we did
        // not touch it
        pkt->pushSenderState(req_state);
        pkt->setDest(dest);
    }

    return successful;
}

void
TagXBar::recvTimingSnoopReq(PacketPtr pkt)
{
    DPRINTFN("TagXBar::recvTimingSnoopReq()\n");
    bool needsResponse = pkt->needsResponse();
    bool memInhibitAsserted = pkt->memInhibitAsserted();

    if (!memInhibitAsserted && needsResponse)
        pkt->pushSenderState(new RequestState(pkt->getSrc()));

    // Snoop request always success
    slavePort.sendTimingSnoopReq(pkt);
}

bool
TagXBar::recvTimingSnoopResp(PacketPtr pkt)
{
    DPRINTFN("TagXBar::recvTimingSnoopResp()\n");
    RequestState *req_state =
        dynamic_cast<RequestState*>(pkt->popSenderState());
    // panic if failed to restore initial sender state
    panic_if(!req_state,
             "TagXBar %s got a response without sender state.",
             name());

    PortID dest = pkt->getDest();

    pkt->setDest(req_state->origSrc);
    pkt->firstWordDelay = pkt->lastWordDelay = 0;

    // Attemp to send the packet
    bool successful = masterPort.sendTimingSnoopResp(pkt);

    // If packet successfully sent delete the sender state otherwise
    // restore state
    if (successful) {
        delete req_state;
    } else {
        // Don't delete anything and let the packet look like we did
        // not touch it
        pkt->pushSenderState(req_state);
        pkt->setDest(dest);
    }

    return successful;
}

TagXBar *
TagXBarParams::create()
{
    return new TagXBar(this);
}

