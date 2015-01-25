/*
 * Copyright (c) 2015 Institute of Computing Technology, CAS
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

#include "dev/pardg5v_iohub.hh"


// Hack 1/3: hidden master/default port count to NoncoherentXBar
static int mcnt, dcnt;
PARDg5VIOHub::PARDg5VIOHub(PARDg5VIOHubParams *p)
    : // Hack 2/3: hidden master/default port count to NoncoherentXBar
      NoncoherentXBar((mcnt = p->port_master_connection_count,
                       dcnt = p->port_default_connection_count,
                       p->port_master_connection_count=0,
                       p->port_default_connection_count=0,
                       p)),
      cp(p->cp)
{
    // Hack 3/3: restore master/default port cont
    p->port_master_connection_count = mcnt;
    p->port_default_connection_count = dcnt;

    // create the ports based on the size of the master and slave
    // vector ports, and the presence of the default port, the ports
    // are enumerated starting from zero
    for (int i = 0; i < p->port_master_connection_count; ++i) {
        std::string portName = csprintf("%s.master[%d]", name(), i);
        MasterPort* bp = new IOHubMasterPort(portName, *this, i);
        masterPorts.push_back(bp);
        reqLayers.push_back(new ReqLayer(*bp, *this,
                                         csprintf(".reqLayer%d", i)));
    }

    // see if we have a default slave device connected and if so add
    // our corresponding master port
    if (p->port_default_connection_count) {
        defaultPortID = masterPorts.size();
        std::string portName = name() + ".default";
        MasterPort* bp = new IOHubMasterPort(portName, *this,
                                             defaultPortID);
        masterPorts.push_back(bp);
        reqLayers.push_back(new ReqLayer(*bp, *this, csprintf(".reqLayer%d",
                                                              defaultPortID)));
    }
}

PARDg5VIOHub::~PARDg5VIOHub()
{
}

Addr
PARDg5VIOHub::remapAddr(Addr addr, uint16_t DSid) const
{
    Addr base;
    Addr remapped;

    if (!cp->remapAddr(DSid, addr, &base, &remapped))
        return addr;
    addr = remapped + (addr-base);
    return addr;
}

PARDg5VIOHubRemapper *
PARDg5VIOHubRemapperParams::create()
{
    return new PARDg5VIOHubRemapper(this);
}

PARDg5VIOHub *
PARDg5VIOHubParams::create()
{
    return new PARDg5VIOHub(this);
}
