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
 * Definition of PARDg5V system control plane.
 */

#include "arch/x86/pardg5v_system_cp.hh"
#include "debug/ControlPlane.hh"

PARDg5VSystemCP::PARDg5VSystemCP(const Params *p)
    : ControlPlane(p),
      param_table_entries(p->param_table_entries),
      stat_table_entries(p->stat_table_entries)
{
    // Construct SystemInfo struct
    sysinfo.cpuNr = 8;
    sysinfo.memSize = (uint64_t)8<<30;

    // Allocate ConfigMemory
    configMem = new char[CFGMEM_SIZE];

    // Allocate ConfigTable
    paramTable = new struct ParamEntry[param_table_entries];
    statTable  = new struct StatEntry[stat_table_entries];
    memset(paramTable, 0, sizeof(struct ParamEntry)*param_table_entries);
    memset(statTable,  0, sizeof(struct StatEntry) *stat_table_entries);
}

PARDg5VSystemCP::~PARDg5VSystemCP()
{
    delete[] configMem;
    delete[] paramTable;
    delete[] statTable;
}

const Segment * 
PARDg5VSystemCP::getSegment(uint16_t DSid, int idx)
{
    assert(idx < PARAMTABLE_SEGMENT_COUNT);

    for (int i=0; i<param_table_entries; i++) {
        if ((paramTable[i].flags & FLAG_VALID)
              && (paramTable[i].DSid == DSid)) {
            return &paramTable[i].segs[idx];
        }
    }

    return NULL;
}

const uint8_t *
PARDg5VSystemCP::getConfigMem(int offset, int size)
{
    if (offset+size > CFGMEM_SIZE)
        return NULL;
    return (const uint8_t *)configMem + offset;
}

int
PARDg5VSystemCP::getCpuMask(uint16_t DSid, uint64_t *mask)
{
    assert(mask);
    for (int i=0; i<param_table_entries; i++) {
        if ((paramTable[i].flags & FLAG_VALID)
              && (paramTable[i].DSid == DSid)) {
            *mask = paramTable[i].cpuMask;
            return 0;
        }
    }
    return -ENOENT;
}

uint64_t *
PARDg5VSystemCP::parseAddr(uint32_t addr)
{
    char *ptr = NULL;
    int offset;

    switch (addr & ADDRTYPE_MASK)
    {
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
              case CFGTBL_TYPE_STAT:
                if ((row < stat_table_entries) &&
                    (offset <= sizeof(struct StatEntry) - sizeof(uint64_t)))
                    ptr = (char *)&statTable[row];
                break;
            }
        }
        break;
      case ADDRTYPE_CFGMEM:
        offset = cfgmem_addr2offset(addr);
        if (offset <= CFGMEM_SIZE - sizeof(uint64_t))
            ptr = configMem;
        break;
      case ADDRTYPE_SYSINFO:
        offset = sysinfo_addr2offset(addr);
        if (offset <= sizeof(sysinfo) - sizeof(uint64_t))
            ptr = (char *)&sysinfo;
        break;
    }

    return (ptr ? ((uint64_t *)(ptr + offset)) : NULL);
}

uint64_t
PARDg5VSystemCP::queryTable(uint16_t DSid, uint32_t addr)
{
    uint64_t *pdata;

    DPRINTF(ControlPlane, "queryTable(DSid=%d, addr=0x%x)\n",
            DSid, addr);

    pdata = parseAddr(addr);
    if (!pdata) {
        warn("PARDg5VSystemCP: unknown addr 0x%x", addr);
        return 0xFFFFFFFFFFFFFFFF;
    }

    return *pdata;
}

void
PARDg5VSystemCP::updateTable(uint16_t DSid, uint32_t addr, uint64_t data)
{
    uint64_t *pdata;

    DPRINTF(ControlPlane, "updateTable(DSid=%d, addr=0x%x, data=0x%x)\n",
            DSid, addr, data);

    pdata = parseAddr(addr);
    if (!pdata) {
        warn("PARDg5VSystemCP: unknown addr 0x%x", addr);
        return;
    }

    *pdata = data;
}

PARDg5VSystemCP *
PARDg5VSystemCPParams::create()
{
    return new PARDg5VSystemCP(this);
}
