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
#include "debug/CoherentTagXBar.hh"
#include "mem/coherent_tag_xbar.hh"
#include "sim/system.hh"

CoherentTagXBar::CoherentTagXBar(const CoherentTagXBarParams *p,
                         const unsigned int port_slave_connection_count)
    : CoherentXBar(p),
      DSid(p->DSid), DSid_base_addr(p->DSid_base_addr)
{
    // create the slave ports, because they are faked in CoherentXBar
    // see CoherentTagXBarParams::create()
    for (int i = 0; i < port_slave_connection_count; ++i) {
        std::string portName = csprintf("%s.slave[%d]", name(), i);
        SlavePort* bp = new CoherentTagXBarSlavePort(portName, *this, i);
        slavePorts.push_back(bp);
        respLayers.push_back(new RespLayer(*bp, *this,
                                           csprintf(".respLayer%d", i)));
        snoopRespPorts.push_back(new SnoopRespPort(*bp, *this));
    }
}

CoherentTagXBar::~CoherentTagXBar()
{
}

CoherentTagXBar *
CoherentTagXBarParams::create()
{
    // Fake slave port to CoherentXBar
    unsigned int fake_count = this->port_slave_connection_count;
    this->port_slave_connection_count=0;
    return new CoherentTagXBar(this, fake_count);
}

