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

#ifndef __DEV_CELLX_ICH_HH__
#define __DEV_CELLX_ICH_HH__

#include "dev/cellx/ich_cp.hh"
#include "mem/bridge.hh"
#include "mem/noncoherent_xbar.hh"
#include "mem/tag_addr_mapper.hh"
#include "params/PARDg5VICH.hh"
#include "params/PARDg5VICHRemapper.hh"

namespace X86ISA
{
    class I8254;
    class Cmos;
    class Speaker;
    class I82094AX;
}

class PARDg5VICH : public Bridge
{
    friend class PARDg5VICHRemapper;
    friend class X86ISA::I82094AX;

  protected:

    Platform * platform;

    // Control Plane
    PARDg5VICHCP *cp;

  public:
    X86ISA::I82094AX * ioApic;
    X86ISA::Cmos * cmos;
    std::vector<X86ISA::I8254 *> pits;
    std::vector<X86ISA::I8250X *> serials;

  protected:

    Addr remapAddr(Addr addr, uint16_t DSid) const;

  public:
    typedef PARDg5VICHParams Params;
    PARDg5VICH(Params *p);

    virtual void init();

    const Params *
    params() const
    {
        return dynamic_cast<const Params *>(_params);
    }
};

class PARDg5VICHRemapper : public TagAddrMapper
{
  private:
    /**
     * ICH this mapper belong to
     */
    PARDg5VICH *ich;
    /**
     * All Memory AddrRange
     */
    AddrRangeList AllMemory;

  public:
    PARDg5VICHRemapper(const PARDg5VICHRemapperParams *p)
        : TagAddrMapper(p), ich(p->ich)
    {
        AllMemory.push_back(AddrRange(0, (Addr)(-1)));
    }
    virtual ~PARDg5VICHRemapper() { }

    virtual AddrRangeList getAddrRanges() const
    { return AllMemory; }

  protected:
    virtual Addr remapAddr(Addr addr, uint16_t DSid) const
    { return ich->remapAddr(addr, DSid); }
};

#endif // __DEV_CELLX_ICH_HH__
