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
 * Declaration of PARDg5V system control plane.
 */

/**
 * PARDg5-V Control Plane Address Mapping (32-bit address)
 *
 * - ConfigTable
 *     31                                   0
 *     +--+--+----------+--------+----------+
 *     |00|XX|**********| row_id |  offset  +
 *     +--+--+----------+--------+----------+
 *     XX : 00 - ParamTable
 *          01 - StatTable
 *          10 - TriggerTable
 *
 * - ConfigMemory: 16MB
 *     31 30    24                        0
 *     +--+------+------------------------+
 *     |01|******|          16MB          |
 *     +--+------|------------------------+
 *
 * - SystemInfo: 256Byte
 *     31 30                     8        0
 *     +--+----------------------+--------+
 *     |10|**********************|  256B  |
 *     +--+----------------------+--------+
 *
 */

#ifndef __ARCH_X86_PARDG5V_SYSTEM_CP_HH__
#define __ARCH_X86_PARDG5V_SYSTEM_CP_HH__

#include "params/PARDg5VSystemCP.hh"
#include "prm/ControlPlane.hh"

#define CFGMEM_BITS	24
#define CFGMEM_SIZE	(1<<CFGMEM_BITS)


struct Segment {
    uint64_t base;
    uint32_t size;
    uint32_t offset;
};

#define PARAMTABLE_SEGMENT_COUNT	4

#define FLAG_VALID			0x8000

struct ParamEntry {
    uint16_t DSid;
    uint16_t flags;
    uint32_t __padding;
    uint64_t cpuMask;
    uint64_t bsp_entry_addr;
    struct Segment segs[PARAMTABLE_SEGMENT_COUNT];
};

struct StatEntry {
    uint16_t DSid;
    uint16_t flags;
    uint32_t __padding;
    uint64_t __padding2;
};

struct SystemInfo {
    uint8_t cpuNr;
    uint64_t memSize;
};

class PARDg5VSystemCP : public ControlPlane
{
  protected:
    int param_table_entries;
    int stat_table_entries;

    struct ParamEntry *paramTable;
    struct StatEntry  *statTable;
    struct SystemInfo sysinfo;
    char *configMem;

  public:
    typedef PARDg5VSystemCPParams Params;
    PARDg5VSystemCP(const Params *p);
    ~PARDg5VSystemCP();

  public:
    int getSegmentCount(uint16_t DSid) { return PARAMTABLE_SEGMENT_COUNT; }
    const Segment * getSegment(uint16_t DSid, int idx);
    int getCpuMask(uint16_t DSid, uint64_t *mask);
    const uint8_t *getConfigMem(int offset, int size);

    virtual uint64_t queryTable(uint16_t DSid, uint32_t addr);
    virtual void updateTable(uint16_t DSid, uint32_t addr, uint64_t data);

  private:
    uint64_t * parseAddr(uint32_t addr);

  protected:
    const Params * param() const
    { return dynamic_cast<const Params *>(_params); }
};

#endif	// __ARCH_X86_PARDG5V_SYSTEM_CP_HH__
