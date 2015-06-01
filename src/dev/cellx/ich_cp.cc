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

#define _IO(x) x86IOAddress(x)

PARDg5VICHCP::PARDg5VICHCP(const Params *p)
    : ControlPlane(p),
      param_table_entries(p->param_table_entries)
{
    paramTable = new struct ParamEntry[param_table_entries];
    memset(paramTable, -1, sizeof(struct ParamEntry)*param_table_entries);
    for (int i=0; i<param_table_entries; i++)
        paramTable[i].flags = 0;

    compoments_nr = 4;
    compoments = new struct ICH_COMPOMENT[4][ICH_COMPOMENTS_NR] {
        // COM,             CMOS,            PIT,             I/O APIC
        {{ _IO(0x3f8), 8, 4 }, { _IO(0x70), 4, 8 }, { _IO(0x40), 4, 0 }, { 0xFEC00000, 0x14, 0xFFFF }},
        {{ _IO(0x2f8), 8, 5 }, { _IO(0x70), 4, 8 }, { _IO(0x44), 4, 1 }, { 0xFEC00000, 0x14, 0xFFFF }},
        {{ _IO(0x3e8), 8, 6 }, { _IO(0x70), 4, 8 }, { _IO(0x48), 4, 2 }, { 0xFEC00000, 0x14, 0xFFFF }},
        {{ _IO(0x2e8), 8, 7 }, { _IO(0x70), 4, 8 }, { _IO(0x4C), 4, 3 }, { 0xFEC00000, 0x14, 0xFFFF }},
    };
}

PARDg5VICHCP::~PARDg5VICHCP()
{
    delete[] paramTable;
    delete[] compoments;
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
    char *ptr = NULL;
    int offset;

    switch (addr & ADDRTYPE_MASK) {
    // Access IOHub ConfigTable
    case ADDRTYPE_CFGTBL:
        {
            int row = cfgtbl_addr2row(addr);
            offset = cfgtbl_addr2offset(addr);

            switch (cfgtbl_addr2type(addr)) {
              case CFGTBL_TYPE_PARAM:
                if ((row < param_table_entries) &&
                    (offset <= sizeof(struct ParamEntry) - sizeof(uint64_t)))
                    ptr = (char *)&paramTable[row];
                break;
            }
        }
        break;
    // Access IOHub Info
    case ADDRTYPE_SYSINFO:
        ptr = NULL;
        break;
    }

    return (ptr ? ((uint64_t *)(ptr + offset)) : NULL);
}

bool
PARDg5VICHCP::remapAddr(const uint16_t DSid, const Addr addr,
                          Addr *base, Addr*remapped) const
{
    struct ParamEntry *pParamEntry = NULL;
    struct ICH_COMPOMENT *pComp = NULL;

    for (int i=0; i<param_table_entries; i++) {
        if (paramTable[i].DSid == DSid) {
            pParamEntry = &paramTable[i];
            break;
        }
    }

    // No such DSid or ICH not selected for this DSid, return non-exist
    if (pParamEntry && pParamEntry->selected >= 4)
        pParamEntry->selected = 0xFFFF;
    if (!pParamEntry || (pParamEntry->selected == 0xFFFF)) {
        *base = addr;
        *remapped = x86IOAddress(0);
        return true;
    }
    pComp = compoments[pParamEntry->selected];

    // Check ICH compoments
    for (int i=0; i<ICH_COMPOMENTS_NR; i++) {
        if (addr >= pParamEntry->regbase[i] &&
            addr <  pParamEntry->regbase[i] + pComp[i].regsize)
        {
            *base     = pParamEntry->regbase[i];
            *remapped = pComp[i].regbase;
            return true;
        }
    }

    // No other address
    *base = addr;
    *remapped = x86IOAddress(0);
    return true;
}

bool
PARDg5VICHCP::remapInterrupt(
    int line,
    std::vector<std::pair<uint16_t, int> > &targets)
{
    targets.clear();
    for (int i=0; i<param_table_entries; i++) {
        if (paramTable[i].intlines[line] != -1) {
            targets.push_back(std::pair<uint16_t, int>(
                paramTable[i].DSid,
                paramTable[i].intlines[line]));
        }
    }

    return !targets.empty();
}

PARDg5VICHCP *
PARDg5VICHCPParams::create()
{
    return new PARDg5VICHCP(this);
}

