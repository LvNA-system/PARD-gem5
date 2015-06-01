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
#include "mem/mem_object.hh"
#include "dev/pard/dma_device.hh"
#include "dev/pard/iohub_cp.hh"
#include "dev/pard/iohub.hh"
#include "dev/pcireg.h"               // for PCI_CONFIG_SIZE

using namespace X86ISA;


PARDg5VIOHubCP::PARDg5VIOHubCP(const Params *p)
    : ControlPlane(p),
      param_table_entries(p->param_table_entries),
      stat_table_entries(p->stat_table_entries)
{
    // Construct IOHubInfo struct
    memset(&ioInfo, 0, sizeof(ioInfo));
    ioInfo.device_ptr = 0xFFFF;
    ioInfo.device_nr = 0;

    // Allocate ConfigTable
    paramTable = new struct ParamEntry[param_table_entries];
    statTable = new struct StatEntry[stat_table_entries];
    memset(paramTable, 0, sizeof(struct ParamEntry)*param_table_entries);
    memset(statTable,  0, sizeof(struct StatEntry) *stat_table_entries);
}

PARDg5VIOHubCP::~PARDg5VIOHubCP()
{
    delete[] statTable;
    delete[] paramTable;
}

void
PARDg5VIOHubCP::regPARDg5VIOHub(PARDg5VIOHub *_iohub)
{
    panic_if(!iohub, "%s already reg to %s\n", name().c_str(), iohub->name().c_str());
    iohub = _iohub;
}

uint64_t
PARDg5VIOHubCP::queryTable(uint16_t DSid, uint32_t addr)
{
    uint64_t *pdata;
    DPRINTF(ControlPlane, "queryTable(DSid=%d, addr=0x%x)\n",
            DSid, addr);
    pdata = parseAddr(addr);
    if (!pdata) {
        warn("PARDg5VIOHubCP: unknown addr 0x%x", addr);
        return 0xFFFFFFFFFFFFFFFF;
    }
    return *pdata;
}

void
PARDg5VIOHubCP::updateTable(uint16_t DSid, uint32_t addr, uint64_t data)
{
    uint64_t *pdata;
    uint64_t old_data;

    DPRINTF(ControlPlane, "updateTable(DSid=%d, addr=0x%x, data=0x%x)\n",
            DSid, addr, data);

    pdata = parseAddr(addr);
    if (!pdata) {
        warn("PARDg5VIOHubCP: unknown addr 0x%x", addr);
        return;
    }

    old_data = *pdata;
    *pdata = data;

    if ((char *)pdata >= (char *)paramTable &&
        (char *)pdata <  (char *)paramTable+param_table_entries*sizeof(struct ParamEntry))
    {
        int idx = ((uint64_t)pdata - (uint64_t)paramTable)/sizeof(struct ParamEntry);
        if ((char *)pdata <= (char *)&paramTable[idx].device_mask &&
            (char *)pdata+sizeof(*pdata)
              >= (char *)&paramTable[idx].device_mask+sizeof(paramTable[idx].device_mask))
        {
            uint32_t *p_old_device_mask = (uint32_t *)((char *)&old_data + offsetof(struct ParamEntry, device_mask) - ((char *)pdata-(char *)&paramTable[idx]));
            uint32_t *p_device_mask = &paramTable[idx].device_mask;

            if (paramTable[idx].flags & FLAG_VALID) {
                uint32_t changed_mask = *p_device_mask ^ *p_old_device_mask;
                for (int i=0; i<32; i++) {
                    if (changed_mask & (uint32_t)1<<i) {
                        if (*p_device_mask & (uint32_t)1<<i) {
                            DPRINTFN("ASSIGN DSid#%d ==> dev#%d.\n", paramTable[idx].DSid, i);
                            static_cast<PARDg5VDmaDevice *>(iohub->getPciDevice(i)->owner)->setDSid(paramTable[idx].DSid);
                        }
                        else {
                            DPRINTFN("RELEASE DSid#%d ==> dev#%d.\n", paramTable[idx].DSid, i);
                            static_cast<PARDg5VDmaDevice *>(iohub->getPciDevice(i)->owner)->setDSid(-1);
                        }
                    }
                }
            }
        }
    }
}

uint64_t *
PARDg5VIOHubCP::parseAddr(uint32_t addr)
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
              case CFGTBL_TYPE_STAT:
                if ((row < stat_table_entries) &&
                    (offset <= sizeof(struct StatEntry) - sizeof(uint64_t)))
                    ptr = (char *)&statTable[row];
                break;
            }
        }
        break;
    // Access IOHub Info
    case ADDRTYPE_SYSINFO:
        offset = sysinfo_addr2offset(addr);
        if (offset <= sizeof(ioInfo)-sizeof(uint64_t))
            ptr = (char *)&ioInfo;
        break;
    }

    return (ptr ? ((uint64_t *)(ptr + offset)) : NULL);
}

void
PARDg5VIOHubCP::recvDeviceChange(
    const std::vector<struct PCI_DEVICE *> &devices)
{
    // ioInfo header
    ioInfo.device_nr = devices.size();
    ioInfo.device_ptr = (devices.size() == 0) ? 0xFFFF
                        : (uint64_t)(&((struct IOHubInfo *)NULL)->devices[0]);
    if (ioInfo.device_nr == 0) {
        warn("No device registered to PARDg5VIOHubCP.\n");
        return;
    }

    // build iohub device info
    struct IOHubDevice *devInfo = NULL;
    for (int id=0; id<devices.size(); id++) {
        auto dev = devices[id];
        devInfo = &ioInfo.devices[id];

        devInfo->next_ptr  = (id == devices.size()-1) ? 0xFFFF 
                             : (uint64_t)
                               (&((struct IOHubInfo *)NULL)->devices[id+1]);
        devInfo->id        = id;
        devInfo->flags     = dev->pard_compatible ? 1 : 0;
        devInfo->pciid     = dev->pciid;
        devInfo->interrupt = dev->interruptLine;
        strncpy(devInfo->ident, dev->owner->name().c_str(), 32);
    }
}

uint32_t
PARDg5VIOHubCP::getDeviceMask(uint16_t DSid)
{
    uint32_t device_mask = 0;

    for (int i=0; i<param_table_entries; i++) {
        if ((paramTable[i].flags & FLAG_VALID) &&
            (paramTable[i].DSid == DSid)) {
            device_mask = paramTable[i].device_mask;
            break;
        }
    }

    return device_mask;
}


PARDg5VIOHubCP *
PARDg5VIOHubCPParams::create()
{
    return new PARDg5VIOHubCP(this);
}

