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
 * Declaration of PARDg5V ICH control plane.
 */

/**
 * TODO: PARDg5-V ICH Control Plane Address Mapping (32-bit address)
 *
*/

#ifndef __DEV_CELLX_ICHCP_HH__
#define __DEV_CELLX_ICHCP_HH__

#include "base/addr_range.hh"
#include "params/PARDg5VICHCP.hh"
#include "prm/ControlPlane.hh"


#define ICH_COMPOMENTS_NR       4

/**
 * Config Table
 */
struct ParamEntry {
    uint32_t flags;
    uint16_t DSid;
    uint16_t selected;
    uint64_t regbase[ICH_COMPOMENTS_NR];
    int intlines[24];
};

/**
 * SystemInfo Table
 */
struct ICH_COMPOMENT {
    uint64_t regbase;
    uint32_t regsize;
    uint32_t intline;
};

struct ICHInfo {
    struct ICH_COMPOMENT compoments[][ICH_COMPOMENTS_NR];
};

class PARDg5VICHCP : public ControlPlane
{
  protected:
    int param_table_entries;
    int compoments_nr;
    struct ParamEntry *paramTable;
    struct ICH_COMPOMENT (*compoments)[ICH_COMPOMENTS_NR];

  public:
    typedef PARDg5VICHCPParams Params;
    PARDg5VICHCP(const Params *p);
    ~PARDg5VICHCP();

  public:
    virtual uint64_t queryTable(uint16_t DSid, uint32_t addr);
    virtual void updateTable(uint16_t DSid, uint32_t addr, uint64_t data);

    bool remapAddr(const uint16_t DSid, const Addr addr,
                   Addr *base, Addr*remapped) const;
    bool remapInterrupt(int line,
                   std::vector<std::pair<uint16_t, int> > &targets);

  private:
    uint64_t *parseAddr(uint32_t addr);

  protected:
    const Params *param() const
    { return dynamic_cast<const Params *>(_params); }
};

#endif	// __DEV_CELLX_ICHCP_HH__
