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
#include "dev/pardg5v_iohub_cp.hh"

using namespace X86ISA;

PARDg5VIOHubCP::PARDg5VIOHubCP(const Params *p)
    : ControlPlane(p)
{
    ioInfoData = new char[IOHUB_INFO_SIZE];
    memset(ioInfoData, 0, IOHUB_INFO_SIZE);
}

PARDg5VIOHubCP::~PARDg5VIOHubCP()
{
    delete[] ioInfoData;
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

    DPRINTF(ControlPlane, "updateTable(DSid=%d, addr=0x%x, data=0x%x)\n",
            DSid, addr, data);

    pdata = parseAddr(addr);
    if (!pdata) {
        warn("PARDg5VIOHubCP: unknown addr 0x%x", addr);
        return;
    }

    *pdata = data;
}

uint64_t *
PARDg5VIOHubCP::parseAddr(uint32_t addr)
{
    // Access IOHub Info
    if (((addr& 0xC0000000) == 0x80000000) &&
         (addr&~0xC0000000) < IOHUB_INFO_SIZE)
    {
        return (uint64_t *)&ioInfoData[addr & ~0xC0000000];
    }

    return NULL;
}

void
PARDg5VIOHubCP::recvDeviceChange(std::map<PortID, AddrRangeList> devices)
{
    char *ptr = ioInfoData;
    struct IOHubInfo *ioInfo = (struct IOHubInfo *)ioInfoData;

    ioInfo->device_cnt = devices.size();
    ptr = (char *)ioInfo->devices;

    for (auto dev : devices) {
        struct IOHubDevice *devInfo = (struct IOHubDevice *)ptr;
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

/*
    {	// DEBUG ONLY, print out ioInfoData
        ioInfo = (struct IOHubInfo *)ioInfoData;
        ptr = (char *)ioInfo->devices;
        printf("Total Devices: %d\n", ioInfo->device_cnt);
        for (int i=0; i<ioInfo->device_cnt; i++) {
            struct IOHubDevice *devInfo = (struct IOHubDevice *)ptr;
            ptr = (char *)devInfo->ranges;
            printf("  Device #%d @ Port%d:\n", i, devInfo->id);
            for (int j=0; j<devInfo->cnt; j++) {
                struct IOAddrRange *rangeInfo = (struct IOAddrRange *)ptr;
                ptr = (char *)(rangeInfo+1);
                printf("    [0x%lx, 0x%lx]\n", rangeInfo->base, rangeInfo->base+rangeInfo->size-1);
            }
        }
        printf("\n");
    }
*/
}

bool
PARDg5VIOHubCP::remapAddr(const uint16_t DSid, const Addr addr,
                          Addr *base, Addr*remapped) const
{
/*
    if (addr == x86IOAddress(0x3f8)) {
        *base = x86IOAddress(0x3f8);
        switch (DSid) {
          case 0:
            *remapped = x86IOAddress(0x3f8);
            break;
          case 1:
            *remapped = x86IOAddress(0x2f8);
            break;
          case 2:
            *remapped = x86IOAddress(0x3e8);
            break;
          case 3:
            *remapped = x86IOAddress(0x2e8);
            break;
          default:
            return false;
        }
        return true;
    }
*/

    return false;

/*
    for (int i=0; i<param_table_entries; i++) {
        if (paramTable[i].DSid == DSid) {
            for (int j=0; j<5; j++) {
                if (VALID(paramTable[i].ranges[j]) &&
                    in_range(paramTable[i].ranges[j], addr)) {
                    return paramTable[i].ranges[j].remapped;
                }
            }
        }
    }
*/
}


PARDg5VIOHubCP *
PARDg5VIOHubCPParams::create()
{
    return new PARDg5VIOHubCP(this);
}

