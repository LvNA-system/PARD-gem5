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

#include "arch/x86/x86_traits.hh"
#include "debug/ControlPlane.hh"
#include "dev/cellx/ich_cp.hh"

using namespace X86ISA;

PARDg5VICHCP::PARDg5VICHCP(const Params *p)
    : ControlPlane(p)
{
    ioInfoData = new char[IOHUB_INFO_SIZE];
    memset(ioInfoData, 0, IOHUB_INFO_SIZE);
}

PARDg5VICHCP::~PARDg5VICHCP()
{
    delete[] ioInfoData;
}

uint64_t
PARDg5VICHCP::queryTable(uint16_t DSid, uint32_t addr)
{
    uint64_t *pdata;
    DPRINTF(ControlPlane, "queryTable(DSid=%d, addr=0x%x)\n",
            DSid, addr);
    pdata = parseAddr(addr);
    if (!pdata) {
        warn("PARDg5VICHCP: unknown addr 0x%x", addr);
        return 0xFFFFFFFFFFFFFFFF;
    }
    return *pdata;
}

void
PARDg5VICHCP::updateTable(uint16_t DSid, uint32_t addr, uint64_t data)
{
    uint64_t *pdata;

    DPRINTF(ControlPlane, "updateTable(DSid=%d, addr=0x%x, data=0x%x)\n",
            DSid, addr, data);

    pdata = parseAddr(addr);
    if (!pdata) {
        warn("PARDg5VICHCP: unknown addr 0x%x", addr);
        return;
    }

    *pdata = data;
}

uint64_t *
PARDg5VICHCP::parseAddr(uint32_t addr)
{
    // Access ICH Info
    if (((addr& 0xC0000000) == 0x80000000) &&
         (addr&~0xC0000000) < IOHUB_INFO_SIZE)
    {
        return (uint64_t *)&ioInfoData[addr & ~0xC0000000];
    }

    return NULL;
}

void
PARDg5VICHCP::recvDeviceChange(std::map<PortID, AddrRangeList> devices)
{
    char *ptr = ioInfoData;
    struct ICHInfo *ioInfo = (struct ICHInfo *)ioInfoData;

    ioInfo->device_cnt = devices.size();
    ptr = (char *)ioInfo->devices;

    for (auto dev : devices) {
        struct ICHDevice *devInfo = (struct ICHDevice *)ptr;
        ptr = (char *)devInfo->ranges;

        devInfo->id = dev.first;
        devInfo->cnt = dev.second.size();

        for (auto r : dev.second) {
            struct IOAddrRange *rangeInfo = (struct IOAddrRange *)ptr;
            ptr = (char *)(rangeInfo+1);

            rangeInfo->base = r.start();
            rangeInfo->size = r.size();
        }
    }
}

bool
PARDg5VICHCP::remapAddr(const uint16_t DSid, const Addr addr,
                          Addr *base, Addr*remapped) const
{
    // UART, map COM_1 => {COM_1, COM_2, COM_3, COM_4}
    if (addr >= x86IOAddress(0x3f8) && addr<x86IOAddress(0x3f8)+8) {
        Addr remap_base[] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8 };
        *base = x86IOAddress(0x3f8);
        *remapped = x86IOAddress(remap_base[DSid]);
        return true;
    }
    // UART, map COM_2/3/4 => Non-existant
    else if (((addr >= x86IOAddress(0x2f8)) && (addr<x86IOAddress(0x2f8)+8)) ||
             ((addr >= x86IOAddress(0x3e8)) && (addr<x86IOAddress(0x3e8)+8)) ||
             ((addr >= x86IOAddress(0x2e8)) && (addr<x86IOAddress(0x2e8)+8))) {
        *base = addr;
        *remapped = x86IOAddress(0);
        return true;
    }
    // PIT
    else if (addr >= x86IOAddress(0x40) && addr<x86IOAddress(0x40)+4) {
        Addr remap_base[] = { 0x40, 0x44, 0x48, 0x4c };
        *base = x86IOAddress(0x40);
        *remapped = x86IOAddress(remap_base[DSid]);
        return true;
    }
    return false;
}


bool
PARDg5VICHCP::remapInterrupt(
    int line,
    std::vector<std::pair<uint16_t, int> > &targets)
{
    targets.clear();
    /* PIT interrupt */
    if (line == 0)
        targets.push_back(std::pair<uint16_t, int>(0, 2));
    else if (line == 1)
        targets.push_back(std::pair<uint16_t, int>(1, 2));
    else if (line == 2)
        targets.push_back(std::pair<uint16_t, int>(2, 2));
    else if (line == 3)
        targets.push_back(std::pair<uint16_t, int>(3, 2));
    /* UART interrupt */
    else if (line >= 4 && line<=7) {
        DPRINTFN("Receive UART InterruptLine: #%d ==> DSid#%d\n", line, line-4);
        targets.push_back(std::pair<uint16_t, int>(line-4, 4));
    }
    /* XXX: send CMOS interrupt to all */
    else if (line == 8) {
        for (int i=0; i<4; i++)
            targets.push_back(std::pair<uint16_t, int>(i, line));
    }
    /* XXX: send IDE interrupt to DSid=1 */
    else if (line == 10)
        targets.push_back(std::pair<uint16_t, int>(1, line));
    else {
        DPRINTFN("Unmapped InterruptLine: #%d\n", line);
        for (int i=0; i<4; i++)
            targets.push_back(std::pair<uint16_t, int>(i, line));
    }

    return true;
}


PARDg5VICHCP *
PARDg5VICHCPParams::create()
{
    return new PARDg5VICHCP(this);
}

