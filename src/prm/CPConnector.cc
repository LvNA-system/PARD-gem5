/*
 * Copyright (c) 2014 ACS, ICT
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

#include "prm/CPConnector.hh"
#include "prm/ControlPlane.hh"
#include "debug/CPConnector.hh"

CPConnector::CPConnector(Params *p)
    : MemObject(p),
      slavePort(p->name + ".slave", this),
      masterPort(p->name + ".master", this),
      cp(NULL), cmdHandler(NULL),
      cpDevID(p->cp_dev)
{
    memset(&regs, 0xFF, sizeof(regs));
    regs.cpType = p->Type;
    strncpy((char *)regs.cpIdent, p->IDENT.c_str(), 12);
}

void
CPConnector::init()
{
    // make sure both sides are connected and have the same block size
    if (!slavePort.isConnected())
        fatal("Slave port of CPConnector \"%s\" are not connected to a bus.\n",
              name());

    // notify the master side  of our address ranges
    slavePort.sendRangeChange();
}

BaseMasterPort&
CPConnector::getMasterPort(const std::string& if_name, PortID idx)
{
    if (if_name == "master")
        return masterPort;
    else
        // pass it along to our super class
        return MemObject::getMasterPort(if_name, idx);
}

BaseSlavePort&
CPConnector::getSlavePort(const std::string& if_name, PortID idx)
{
    if (if_name == "slave")
        return slavePort;
    else
        // pass it along to our super class
        return MemObject::getSlavePort(if_name, idx);
}

AddrRangeList
CPConnector::getAddrRanges() const
{
    AddrRangeList ranges;
    Addr base = cpDevID * 32;
    ranges.push_back(RangeEx(base, base+31));   
    return ranges;
}


#define ACCESS_DATA(pkt, dataRef, type) do {	\
    assert((pkt)->getSize() == sizeof(type));	\
    assert(sizeof(dataRef) == sizeof(type));	\
    if ((pkt)->isRead())			\
        *((pkt)->getPtr<type>()) = (dataRef);	\
    else if ((pkt)->isWrite())			\
        (dataRef) = *(pkt->getPtr<type>());	\
    else					\
        panic("Error type\n");			\
} while(0)

#define OFFSET_OF(type, field) ((long)(&((type *)0)->field))

Tick
CPConnector::recvAtomic(PacketPtr pkt)
{
    Addr offset = pkt->getAddr() - cpDevID*32;

    DPRINTF(CPConnector, "CPConnector::recvAtomic(offset=0x%lx, size=0x%lx)\n", offset, pkt->getSize());

    // check access right, access must in reg field boundary & size in (1,2,4,8)
    assert((offset == OFFSET_OF(CPConnRegs, cpType))		||
           (offset == OFFSET_OF(CPConnRegs, cpIdent[0]))	||
           (offset == OFFSET_OF(CPConnRegs, cpIdent[4]))	||
           (offset == OFFSET_OF(CPConnRegs, cpCmd))		||
           (offset == OFFSET_OF(CPConnRegs, cpState))		||
           (offset == OFFSET_OF(CPConnRegs, cpLDomID))		||
           (offset == OFFSET_OF(CPConnRegs, cpDestAddr))	||
           (offset == OFFSET_OF(CPConnRegs, cpData)));
    assert(pkt->getSize() == 1 || pkt->getSize() == 2 ||
           pkt->getSize() == 4 || pkt->getSize() == 8);

    // access regs
    if (pkt->isRead())
        memcpy(pkt->getPtr<char *>(), ((char *)&regs) + offset, pkt->getSize());
    else if (pkt->isWrite())
        memcpy(((char *)&regs) + offset, pkt->getPtr<char *>(), pkt->getSize());
    else
        panic("Error type\n");

    // special for cpCmd register
    if (offset == OFFSET_OF(CPConnRegs, cpCmd)) {
        if (regs.cpCmd == 'G')
            regs.cpData = cp->queryTable(regs.cpLDomID, regs.cpDestAddr);
        else if (regs.cpCmd == 'S')
            cp->updateTable(regs.cpLDomID, regs.cpDestAddr, regs.cpData);
        else {
            bool cmd_handled = false;
            if (cmdHandler) {
                cmd_handled = cmdHandler->handleCommand(
                    regs.cpCmd,
                    (uint64_t)regs.cpLDomID,
                    (uint64_t)regs.cpDestAddr,
                    (uint64_t)regs.cpData
                );
            }
            if (!cmd_handled) {
                warn("Unknown ControlPlane Command: 0x%x.\n", regs.cpCmd);
                regs.cpCmd = 0xFF;
            }
        }
        return 3;
    }

    return 1;
}

Tick
CPConnector::recvResponse(PacketPtr pkt)
{
    return 0;
}

CPConnector *
CPConnectorParams::create()
{
    return new CPConnector(this);
}
