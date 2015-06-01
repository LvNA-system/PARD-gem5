# Copyright (c) 2014 Institute of Computing Technology, CAS
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

from m5.objects import *
from Benchmarks import *
from m5.util import *

class CowIdeDisk(PARDg5VIdeDisk):
    image = CowDiskImage(child=RawDiskImage(read_only=True),
                         read_only=False)
    def childImage(self, ci):
        self.image.child.image_file = ci

class MemBus(PARDSystemXBar):
    badaddr_responder = BadAddr()
    default = Self.badaddr_responder.pio

def x86IOAddress(port):
    IO_address_space_base = 0x8000000000000000
    return IO_address_space_base + port

def connectX86ClassicSystem(x86_sys, numCPUs):
    # Constants similar to x86_traits.hh
    IO_address_space_base = 0x8000000000000000
    pci_config_address_space_base = 0xc000000000000000
    interrupts_address_space_base = 0xa000000000000000
    APIC_range_size = 1 << 12;

    x86_sys.membus = MemBus()

    # North Bridge
    x86_sys.iobus = PARDg5VIOHub()
    x86_sys.bridge = Bridge(delay='50ns')
    x86_sys.iobus.attachRemappedMaster(x86_sys.bridge)
    x86_sys.bridge.slave = x86_sys.membus.io_port
    # Allow the bridge to pass through:
    #  1) kernel configured PCI device memory map address: address range
    #     [0xC0000000, 0xFFFF0000). (The upper 64kB are reserved for m5ops.)
    #  2) the bridge to pass through the IO APIC (two pages, already contained in 1),
    #  3) everything in the IO address range up to the local APIC, and
    #  4) then the entire PCI address space and beyond.
    x86_sys.bridge.ranges = \
        [
        AddrRange(0xC0000000, 0xFFFF0000),
        AddrRange(IO_address_space_base,
                  interrupts_address_space_base - 1),
        AddrRange(pci_config_address_space_base,
                  Addr.max)
        ]

    x86_sys.membus.io_ranges = \
        [
        AddrRange(0xC0000000, 0xFFFF0000),
        AddrRange(IO_address_space_base,
                  interrupts_address_space_base - 1),
        AddrRange(pci_config_address_space_base,
                  Addr.max)
        ]
    x86_sys.membus.memory_ranges = [AddrRange(0, 0xBFFFFFFF)]

    # Create a bridge from the IO bus to the memory bus to allow access to
    # the local APIC (two pages)
    x86_sys.apicbridge = Bridge(delay='50ns')
    x86_sys.apicbridge.slave = x86_sys.iobus.master
    x86_sys.apicbridge.master = x86_sys.membus.slave
    x86_sys.apicbridge.ranges = [AddrRange(interrupts_address_space_base,
                                           interrupts_address_space_base +
                                           numCPUs * APIC_range_size
                                           - 1)]

    # connect the io bus
    x86_sys.cellx.attachIO(x86_sys.iobus)

    x86_sys.system_port = x86_sys.membus.slave


def makePARDg5VSystem(mem_mode, numCPUs = 1, mdesc = None):
    self = PARDg5VSystem()

    if not mdesc:
        # generic system
        mdesc = SysConfig()
    self.readfile = mdesc.script()

    self.mem_mode = mem_mode
    self.mem_ranges = [AddrRange(mdesc.mem())]

    # Platform
    self.cellx = CellX()

    # Create and connect the busses required by each memory system
    connectX86ClassicSystem(self, numCPUs)

    self.intrctrl = IntrControl()

    # IDE-0
    ide0_disk0 = CowIdeDisk(driveID='master')
    ide0_disk0.childImage(mdesc.disk())
    self.cellx.ide0 = PARDg5VIdeController(
        disks=[ide0_disk0],
        pci_func=0, pci_dev=6, pci_bus=0,
        InterruptLine = 10,
        InterruptPin = 1)
    self.cellx.ide0.pio    = self.iobus.master
    self.cellx.ide0.config = self.iobus.master
    self.cellx.ide0.dma    = self.iobus.slave

    # IDE-1
    ide1_disk0 = CowIdeDisk(driveID='master')
    ide1_disk0.childImage(mdesc.disk())
    self.cellx.ide1 = PARDg5VIdeController(
        disks=[ide1_disk0],
        pci_func=0, pci_dev=7, pci_bus=0,
        InterruptLine = 11,
        InterruptPin = 1)
    self.cellx.ide1.pio    = self.iobus.master
    self.cellx.ide1.config = self.iobus.master
    self.cellx.ide1.dma    = self.iobus.slave

    # IDE-2
    ide2_disk0 = CowIdeDisk(driveID='master')
    ide2_disk0.childImage(mdesc.disk())
    self.cellx.ide2 = PARDg5VIdeController(
        disks=[ide2_disk0],
        pci_func=0, pci_dev=8, pci_bus=0,
        InterruptLine = 12,
        InterruptPin = 1)
    self.cellx.ide2.pio    = self.iobus.master
    self.cellx.ide2.config = self.iobus.master
    self.cellx.ide2.dma    = self.iobus.slave

    # IDE-3
    ide3_disk0 = CowIdeDisk(driveID='master')
    ide3_disk0.childImage(mdesc.disk())
    self.cellx.ide3 = PARDg5VIdeController(
        disks=[ide3_disk0],
        pci_func=0, pci_dev=9, pci_bus=0,
        InterruptLine = 13,
        InterruptPin = 1)
    self.cellx.ide3.pio    = self.iobus.master
    self.cellx.ide3.config = self.iobus.master
    self.cellx.ide3.dma    = self.iobus.slave

    self.boot_osflags = 'earlyprintk=ttyS0 console=ttyS0 lpj=7999923 ' + \
                        'root=/dev/sda1'
    self.kernel = binary('x86_64-vmlinux-2.6.28.4')

    return self
