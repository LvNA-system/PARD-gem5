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

from m5.defines import buildEnv
from m5.params import *
from m5.proxy import *

from AtomicSimpleCPU import AtomicSimpleCPU
from TimingSimpleCPU import TimingSimpleCPU
from O3CPU import DerivO3CPU
from PARDX86LocalApic import PARDX86LocalApic
from PARDX86TLB import PARDX86TLB
from ClockDomain import *

class TaggedAtomicSimpleCPU(AtomicSimpleCPU):
    itb = PARDX86TLB()
    dtb = PARDX86TLB()

    def createInterruptController(self):
        if buildEnv['TARGET_ISA'] == 'x86':
            self.apic_clk_domain = DerivedClockDomain(clk_domain =
                                                      Parent.clk_domain,
                                                      clk_divider = 16)
            self.interrupts = PARDX86LocalApic(clk_domain = self.apic_clk_domain,
                                           pio_addr=0x2000000000000000)
            _localApic = self.interrupts
        else:
            super(self, TaggedAtomicSimpleCPU).createInterruptController()

class TaggedTimingSimpleCPU(TimingSimpleCPU):
    itb = PARDX86TLB()
    dtb = PARDX86TLB()

    def createInterruptController(self):
        if buildEnv['TARGET_ISA'] == 'x86':
            self.apic_clk_domain = DerivedClockDomain(clk_domain =
                                                      Parent.clk_domain,
                                                      clk_divider = 16)
            self.interrupts = PARDX86LocalApic(clk_domain = self.apic_clk_domain,
                                           pio_addr=0x2000000000000000)
            _localApic = self.interrupts
        else:
            super(self, TaggedAtomicSimpleCPU).createInterruptController()

class TaggedDerivO3CPU(DerivO3CPU):
    def connectCachedPorts(self, bus):
        from TagXBar import TagXBar
        self.tagbus = TagXBar()
        for p in self._cached_ports:
            exec('self.%s = self.tagbus.slave' % p)
        self.tagbus.master = bus.slave

    def connectUncachedPorts(self, bus):
        for p in self._uncached_slave_ports:
            exec('self.%s = bus.master' % p)
        for p in self._uncached_master_ports:
            exec('self.%s = bus.slave' % p)

    def connectAllPorts(self, cached_bus, uncached_bus = None):
        self.connectCachedPorts(cached_bus)
        if not uncached_bus:
            uncached_bus = cached_bus
        self.connectUncachedPorts(uncached_bus)

