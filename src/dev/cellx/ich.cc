/*
 * Copyright (c) 2008 The Regents of The University of Michigan
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
 * Authors: Gabe Black
 */

#include <cassert>

#include "arch/x86/x86_traits.hh"
#include "dev/cellx/cellx.hh"
#include "dev/cellx/ich.hh"
#include "dev/cellx/i82094ax.hh"
#include "dev/x86/i8254.hh"

using namespace X86ISA;

PARDg5VICH::PARDg5VICH(Params *p)
    : Bridge(p),
      platform(p->platform), cp(p->cp),
      ioApic(p->io_apic), cmos(p->cmos), pits(p->pits),
      serials(p->serials)
{
    // Let the platform know where we are
    CellX * cellx = dynamic_cast<CellX *>(platform);
    assert(cellx);
    cellx->ich = this;

    // set parent ich of I/O APIC to self
    ioApic->ich = this;
}

void
PARDg5VICH::init()
{
    /*
     * Initialize the timer.
     */
    for (auto pit : pits) {
        I8254 & timer = *pit;
        //Timer 0, mode 2, no bcd, 16 bit count
        timer.writeControl(0x34);
        //Timer 0, latch command
        timer.writeControl(0x00);
        //Write a 16 bit count of 0
        timer.writeCounter(0, 0);
        timer.writeCounter(0, 0);
    }

    /*
     * Initialize the I/O APIC.
     */
    I82094AX::RedirTableEntry entry = 0;
    entry.deliveryMode = DeliveryMode::ExtInt;
    entry.vector = 0x20;
    ioApic->writeReg(0x10, entry.bottomDW);
    ioApic->writeReg(0x11, entry.topDW);
    entry.deliveryMode = DeliveryMode::Fixed;
    entry.vector = 0x24;
    ioApic->writeReg(0x18, entry.bottomDW);
    ioApic->writeReg(0x19, entry.topDW);
    entry.mask = 1;
    entry.vector = 0x21;
    ioApic->writeReg(0x12, entry.bottomDW);
    ioApic->writeReg(0x13, entry.topDW);
    entry.vector = 0x20;
    ioApic->writeReg(0x14, entry.bottomDW);
    ioApic->writeReg(0x15, entry.topDW);
    entry.vector = 0x28;
    ioApic->writeReg(0x20, entry.bottomDW);
    ioApic->writeReg(0x21, entry.topDW);
    entry.vector = 0x2C;
    ioApic->writeReg(0x28, entry.bottomDW);
    ioApic->writeReg(0x29, entry.topDW);
    entry.vector = 0x2E;
    ioApic->writeReg(0x2C, entry.bottomDW);
    ioApic->writeReg(0x2D, entry.topDW);
    entry.vector = 0x30;
    ioApic->writeReg(0x30, entry.bottomDW);
    ioApic->writeReg(0x31, entry.topDW);

    /*
     * Bridge initialize
     */
    Bridge::init();
}

Addr
PARDg5VICH::remapAddr(Addr addr, uint16_t DSid) const
{
    Addr base;
    Addr remapped;

    if (!cp->remapAddr(DSid, addr, &base, &remapped))
        return addr;
    addr = remapped + (addr-base);
    return addr;
}

PARDg5VICH *
PARDg5VICHParams::create()
{
    return new PARDg5VICH(this);
}

PARDg5VICHRemapper *
PARDg5VICHRemapperParams::create()
{
    return new PARDg5VICHRemapper(this);
}
