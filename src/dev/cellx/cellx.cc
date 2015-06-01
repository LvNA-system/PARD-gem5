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

/** @file
 * Implementation of PARD CellX platform.
 */

#include <deque>
#include <string>
#include <vector>

#include "arch/x86/intmessage.hh"
#include "arch/x86/x86_traits.hh"
#include "config/the_isa.hh"
#include "cpu/intr_control.hh"
#include "dev/cellx/cellx.hh"
#include "dev/cellx/ich.hh"
#include "dev/cellx/i82094ax.hh"
#include "dev/x86/i8254.hh"
#include "dev/terminal.hh"
#include "sim/system.hh"

using namespace std;
using namespace TheISA;

CellX::CellX(const Params *p)
    : Platform(p), system(p->system)
{
    ich = NULL;
}

void
CellX::init()
{
    assert(ich);
}

void
CellX::postConsoleInt()
{
    ich->ioApic->signalInterrupt(4);
}

void
CellX::clearConsoleInt()
{
    warn_once("Don't know what interrupt to clear for console.\n");
    //panic("Need implementation\n");
}

void
CellX::postPciInt(int line)
{
    ich->ioApic->signalInterrupt(line);
}

void
CellX::clearPciInt(int line)
{
    warn_once("Tried to clear PCI interrupt %d\n", line);
}

Addr
CellX::pciToDma(Addr pciAddr) const
{
    return pciAddr;
}

Addr
CellX::calcPciConfigAddr(int bus, int dev, int func)
{
    assert(func < 8);
    assert(dev < 32);
    assert(bus == 0);
    return (PhysAddrPrefixPciConfig | (func << 8) | (dev << 11));
}

uint16_t
CellX::calcPciID(Addr addr)
{
    int bus = 0;
    int dev = ((addr>>11)&0xF);
    int func = ((addr>>8)&0x7);
    return ((bus<<8) | (dev<<4) | func);
}

Addr
CellX::calcPciIOAddr(Addr addr)
{
    return PhysAddrPrefixIO + addr;
}

Addr
CellX::calcPciMemAddr(Addr addr)
{
    return addr;
}

CellX *
CellXParams::create()
{
    return new CellX(this);
}
