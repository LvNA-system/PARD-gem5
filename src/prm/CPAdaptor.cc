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

#include "prm/CPAdaptor.hh"

struct REGISTER_MAP {
    uint32_t base;
    uint32_t size;
    const char * ident;
    uint8_t *ptr;
};

REGISTER_MAP cpRegisterMap[] = {
    {  0x0, 4, "cpType",      NULL },
    {  0x4, 4, "cpIdentLow",  NULL },
    {  0x8, 8, "cpIdentHigh", NULL },
    { 0x10, 1, "command",     NULL },
    { 0x11, 1, "state",       NULL },
    { 0x12, 2, "LDomID",      NULL },
    { 0x14, 4, "destAddr",    NULL },
    { 0x18, 8, "data",        NULL }
};



CPAdaptor::CPAdaptor(Params *p)
    : PciDevice(p),
      masterPort(p->name + ".master", this)
{
}

void
CPAdaptor::init()
{
    PciDevice::init();
}

Tick
CPAdaptor::recvResponse(PacketPtr pkt)
{
    return 0;
}


// method to access cpaRegs
void
CPAdaptor::accessCommand(Addr offset, int size, uint8_t *data, bool read)
{
    switch (offset) {
      case CPA_COMMAND_OFFSET:
        assert (size == sizeof(uint32_t));
        if (read)
            *(uint32_t *)data = cpaRegs.command;
        else
            cpaRegs.command = *(uint32_t *)data;
        break;
      default:
        panic("Invalid CPAdaptor command register offset: %#x data %#x\n",
              offset, *data);
    }
}

// method to access selected CPC's register space
void
CPAdaptor::accessData(Addr offset, int size, uint8_t *data, bool read)
{
    bool check_ok = false;

    for (int i=0; i<sizeof(cpRegisterMap)/sizeof(cpRegisterMap[0]); i++)
    {
        if (offset == cpRegisterMap[i].base) {
            panic_if(size != cpRegisterMap[i].size,
                     "Invalid CPAdaptor request size %#x for %s(%#x required)\n",
                     size, cpRegisterMap[i].ident, cpRegisterMap[i].size);
            check_ok = true;
            break;
        }
    }

    if (!check_ok)
        panic("Invalid CPAdaptor data offset: %#x size: %#x \n", offset, size);

    // Build request to CPN
    Addr paddr = cpaRegs.command.selectCP*32 + offset;
    RequestPtr req = new Request(paddr, size, Request::UNCACHEABLE,
                                 Request::funcMasterId);
    PacketPtr pkt = new Packet(req, read ? MemCmd::ReadReq : MemCmd::WriteReq);
    pkt->dataStatic(data);
    masterPort.sendAtomic(pkt);
    delete pkt;
    delete req;

    // maybe not response immediately, check dispatchAccess()
}

// access dispatcher
void
CPAdaptor::dispatchAccess(PacketPtr pkt, bool read)
{
    int bar;
    Addr addr;
    if (!getBAR(pkt->getAddr(), bar, addr))
        panic("Invalid PCI memory access to unmapped memory.\n");

    if (pkt->getSize() != 1 && pkt->getSize() != 2 &&
            pkt->getSize() != 4 && pkt->getSize() != 8)
    {
         panic("Bad CPAdaptor request size: %d\n", pkt->getSize());
    }

    //Addr addr = pkt->getAddr();
    int size = pkt->getSize();
    uint8_t *dataPtr = pkt->getPtr<uint8_t>();

    if (bar == 0)
        accessCommand(addr, size, dataPtr, read);
    else if (bar == 1)
        accessData(addr, size, dataPtr, read);
    else {
        panic("CPAdaptor access to invalid address; %#x\n", addr);
    }

    pkt->makeAtomicResponse();
}


CPAdaptor *
CPAdaptorParams::create()
{
    return new CPAdaptor(this);
}
