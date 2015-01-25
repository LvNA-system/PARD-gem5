/*
 * Copyright (c) 2014 Institute of Computing Technology, CAS
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
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
 * Authors: Jiuyue Ma
 */

#include "base/chunk_generator.hh"
#include "mem/pard_port_proxy.hh"

void
PARDg5VPortProxy::readBlob(Addr addr, uint8_t *p, int size) const
{
    Request req;

    for (ChunkGenerator gen(addr, size, _cacheLineSize); !gen.done();
         gen.next()) {
        req.setPhys(gen.addr(), gen.size(), 0, Request::funcMasterId);
        req.setDSid(_DSid);
        Packet pkt(&req, MemCmd::ReadReq);
        pkt.dataStatic(p);
        _port.sendFunctional(&pkt);
        p += gen.size();
    }
}

void
PARDg5VPortProxy::writeBlob(Addr addr, const uint8_t *p, int size) const
{
    Request req;

    for (ChunkGenerator gen(addr, size, _cacheLineSize); !gen.done();
         gen.next()) {
        req.setPhys(gen.addr(), gen.size(), 0, Request::funcMasterId);
        req.setDSid(_DSid);
        Packet pkt(&req, MemCmd::WriteReq);
        pkt.dataStaticConst(p);
        _port.sendFunctional(&pkt);
        p += gen.size();
    }
}

