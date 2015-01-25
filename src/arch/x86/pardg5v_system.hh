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
 * Declaration of PARD system.
 */

#ifndef __ARCH_X86_PARDG5V_SYSTEM_HH__
#define __ARCH_X86_PARDG5V_SYSTEM_HH__

#include "arch/x86/pardg5v_system_cp.hh"
#include "mem/pard_port_proxy.hh"
#include "params/PARDg5VSystem.hh"
#include "prm/interfaces.hh"
#include "sim/system.hh"

/**
 * PARDg5VSystem is a X86ISA PARD-hypervisor system. It contains
 * control plane for the whole system, and act as a BIOS for each
 * virtualized system. PRM can program the control plane using CPN.
 * This system can also used to collect system runtime statistics 
 * overviews.
 */
class PARDg5VSystem : public System,
                             ICommandHandler
{
  protected:

      PARDg5VSystemCP *cp;

      /**
       * Port to physical memory used for writing object files into ram at
       * boot. This will hide the variable with same name in class System.
       */
      PARDg5VPortProxy physProxy;

  public:

    typedef PARDg5VSystemParams Params;
    PARDg5VSystem(Params *p);
    ~PARDg5VSystem();

  public:

    virtual void initState();

  public:
    // __override__ ICommandHandler::handleCommand()
    virtual bool handleCommand(int cmd, uint64_t arg1, uint64_t arg2, uint64_t arg3);

  protected:

    void startupLDomain(uint16_t DSid);
    void killLDomain(uint16_t DSid);

    void initBSPState(uint16_t DSid, ThreadContext *tcBSP);
    void writeOutSegment(uint16_t DSid, Addr base, int size, int offset);

  protected:

    const Params *params() const { return (const Params *)_params; }

    virtual Addr fixFuncEventAddr(Addr addr)
    {
        //XXX This may eventually have to do something useful.
        return addr;
    }

};

#endif //__ARCH_X86_PARDG5V_SYSTEM_HH__
