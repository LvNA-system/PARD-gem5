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

/**
 * @file
 * Declaration of PARDg5V IOHub control plane.
 */

/**
 * TODO: PARDg5-V IOHub Control Plane Address Mapping (32-bit address)
 */

#ifndef __DEV_PARDG5V_IOHUB_CP_HH__
#define __DEV_PARDG5V_IOHUB_CP_HH__

#include <set>

#include "base/addr_range.hh"
#include "params/PARDg5VIOHubCP.hh"
#include "prm/ControlPlane.hh"

/**
 * Config Table
 */
struct ParamEntry {
    uint16_t flags;
    uint16_t DSid;
    uint32_t device_mask;
};
#define FLAG_VALID	0x0001

/**
 * State Table
 */
struct StatEntry {	// Indexed by Port
    uint16_t flags;
    uint16_t DSid;
};


/**
 * SystemInfo Table
 */
struct IOHubDevice {
    uint16_t next_ptr;
    uint8_t id;
    uint8_t flags;
    uint16_t pciid;
    uint8_t interrupt;
    uint8_t reserved;
    char ident[32];
};

struct IOHubInfo
{
    uint16_t device_ptr;
    uint8_t  device_nr;
    uint8_t  __res[5];
    struct IOHubDevice devices[32];
};

struct PCI_DEVICE;
class PARDg5VIOHub;

class PARDg5VIOHubCP : public ControlPlane
{
  protected:
    int param_table_entries;
    int stat_table_entries;

    struct StatEntry  *statTable;
    struct ParamEntry *paramTable;
    struct IOHubInfo ioInfo;

    PARDg5VIOHub *iohub;

  public:
    typedef PARDg5VIOHubCPParams Params;
    PARDg5VIOHubCP(const Params *p);
    ~PARDg5VIOHubCP();

    void regPARDg5VIOHub(PARDg5VIOHub *_iohub);

  public:
    uint32_t getDeviceMask(uint16_t DSid);
    void recvDeviceChange(const std::vector<struct PCI_DEVICE *> &devices);

    virtual uint64_t queryTable(uint16_t DSid, uint32_t addr);
    virtual void updateTable(uint16_t DSid, uint32_t addr, uint64_t data);

  private:
    uint64_t *parseAddr(uint32_t addr);

  protected:
    const Params *param() const
    { return dynamic_cast<const Params *>(_params); }
};

#endif	// __DEV_PARDG5V_IOHUB_CP_HH__
