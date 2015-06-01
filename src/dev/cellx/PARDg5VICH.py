# Copyright (c) 2015 Institute of Computing Technology, CAS
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Jiuyue Ma

from m5.params import *
from m5.proxy import *
from Bridge import Bridge
from Cmos import Cmos
from ControlPlane import ControlPlane
from Device import IsaFake
from I82094AX import I82094AX
from I8250X import I8250X
from I8254 import I8254
from Ide import IdeController
from TagAddrMapper import TagAddrMapper
from Terminal import Terminal
from XBar import NoncoherentXBar
from X86IntPin import X86IntLine

def x86IOAddress(port):
    IO_address_space_base = 0x8000000000000000
    return IO_address_space_base + port;


class PARDg5VICHRemapper(TagAddrMapper):
    type = 'PARDg5VICHRemapper'
    cxx_header = "dev/cellx/ich.hh"
    ich = Param.PARDg5VICH(Parent.any, "ICH this mapper belong to")

class PARDg5VICHCP(ControlPlane):
    type = 'PARDg5VICHCP'
    cxx_header = "dev/cellx/ich_cp.hh"

    # CPN address 2:0
    cp_dev = 2
    cp_fun = 0
    # Type 'I' ICH, IDENT: PARDg5VICHCP
    Type = 0x49
    IDENT = "PARDg5VICHCP"

    param_table_entries = Param.Int(32, "Number of parameter table entries")

class PARDg5VICH(Bridge):
    type = 'PARDg5VICH'
    cxx_header = "dev/cellx/ich.hh"
    platform = Param.Platform(Parent.any, "Platform this device is part of")

    # PARDg5VICHCP Control Plane
    cp = Param.PARDg5VICHCP(PARDg5VICHCP(), "Control plane for PARDg5-V ICH")

    # ICH interface logic
    remapper = PARDg5VICHRemapper()
    xbar = NoncoherentXBar()
    ranges = [AddrRange(x86IOAddress(0), size=0x400),
              AddrRange(     0xFEC00000, size=0x14)]

    #
    # Internal Chipsets: CMOS, I/O APIC, PIT*4, UART*4
    #
    _cmos = Cmos(pio_addr=x86IOAddress(0x70))
    _io_apic = I82094AX(pio_addr=0xFEC00000)
    _pits = [I8254(pio_addr=x86IOAddress(0x40)),
             I8254(pio_addr=x86IOAddress(0x44)),
             I8254(pio_addr=x86IOAddress(0x48)),
             I8254(pio_addr=x86IOAddress(0x4C))]
    _serials = [I8250X(pio_addr=x86IOAddress(0x3f8), terminal=Terminal()),
                I8250X(pio_addr=x86IOAddress(0x2f8), terminal=Terminal()),
                I8250X(pio_addr=x86IOAddress(0x3e8), terminal=Terminal()),
                I8250X(pio_addr=x86IOAddress(0x2e8), terminal=Terminal())]

    cmos = Param.Cmos(_cmos, "CMOS memory and real time clock device")
    io_apic = Param.I82094AX(_io_apic, "I/O APIC")
    pits = VectorParam.I8254(_pits, "Programmable interval timer")
    serials = VectorParam.I8250X(_serials, "Serial port and terminal")

    # "Non-existant" devices
    fake_devices = [
        # I8237 DMA
        IsaFake(pio_addr=x86IOAddress(0x0)),
        # I8259 PIC
        IsaFake(pio_addr=x86IOAddress(0x20), pio_size=2), # Master PIC
        IsaFake(pio_addr=x86IOAddress(0xA0), pio_size=2), # Slave PIC
        # I8042 Keyboard/Mouse Controller
        IsaFake(pio_addr=x86IOAddress(0x60), pio_size=1), # data
        IsaFake(pio_addr=x86IOAddress(0x64), pio_size=1), # cmd
        # PcSpeaker
        IsaFake(pio_addr=x86IOAddress(0x61), pio_size=1),
        # "Non-existant" ports used for timing purposes by the linux kernel
        IsaFake(pio_addr=x86IOAddress(0x80), pio_size=1),  # i_dont_exist1
        IsaFake(pio_addr=x86IOAddress(0xed), pio_size=1),  # i_dont_exist2
        # IDE controller
        IsaFake(pio_addr=x86IOAddress(0x3f4), pio_size=3), # BAR0
        IsaFake(pio_addr=x86IOAddress(0x170), pio_size=8), # BAR1
        IsaFake(pio_addr=x86IOAddress(0x374), pio_size=3), # BAR2
        # A device to catch accesses to the non-existant floppy controller.
        IsaFake(pio_addr=x86IOAddress(0x3f2), pio_size=2)]

    def attachIO(self, bus, dma_ports):
        # Route interupt signals
        self.int_lines = [
           X86IntLine(source=self.pits[0].int_pin, sink=self.io_apic.pin(0)),
           X86IntLine(source=self.pits[1].int_pin, sink=self.io_apic.pin(1)),
           X86IntLine(source=self.pits[2].int_pin, sink=self.io_apic.pin(2)),
           X86IntLine(source=self.pits[3].int_pin, sink=self.io_apic.pin(3)),
           X86IntLine(source=self.serials[0].int_pin, sink=self.io_apic.pin(4)),
           X86IntLine(source=self.serials[1].int_pin, sink=self.io_apic.pin(5)),
           X86IntLine(source=self.serials[2].int_pin, sink=self.io_apic.pin(6)),
           X86IntLine(source=self.serials[3].int_pin, sink=self.io_apic.pin(7)),
           X86IntLine(source=self.cmos[0].int_pin, sink=self.io_apic.pin(8))]

        # connect internal devices
        self.io_apic.pio = self.xbar.master
        devices = [self.cmos] + self.pits + self.serials + self.fake_devices
        for dev in devices:
            dev.pio = self.xbar.master

        # connect I/O APIC int master to the bus
        self.io_apic.int_master = bus.slave

        # connect ICH to the bus
        bus.master = self.slave
        self.master = self.remapper.slave
        self.remapper.master = self.xbar.slave
