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

import optparse
import sys

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.util import addToPath, fatal

addToPath('common')
addToPath('../../gem5-offical/configs/common')
addToPath('../../gem5-offical/configs/ruby')

from XFSConfig import *
from SysPaths import *
from Benchmarks import *
import XSimulation
import CacheConfig
import XMemConfig
from Caches import *
import Options

from PARDg5GM import *

def build_pardg5v_system(np):
    if buildEnv['TARGET_ISA'] == "x86":
        pardsys = makePARDg5VSystem(test_mem_mode, options.num_cpus, bm[0])
    else:
        fatal("Incapable of building %s full system!", buildEnv['TARGET_ISA'])

    # Set the cache line size for the entire system
    pardsys.cache_line_size = options.cacheline_size

    # Create a top-level voltage domain
    pardsys.voltage_domain = VoltageDomain(voltage = options.sys_voltage)

    # Create a source clock for the system and set the clock period
    pardsys.clk_domain = SrcClockDomain(clock =  options.sys_clock,
            voltage_domain = pardsys.voltage_domain)

    # Create a CPU voltage domain
    pardsys.cpu_voltage_domain = VoltageDomain()

    # Create a source clock for the CPUs and set the clock period
    pardsys.cpu_clk_domain = SrcClockDomain(clock = options.cpu_clock,
                                             voltage_domain =
                                             pardsys.cpu_voltage_domain)

    if options.kernel is not None:
        pardsys.kernel = binary(options.kernel)

    if options.script is not None:
        pardsys.readfile = options.script

    pardsys.init_param = options.init_param

    # For now, assign all the CPUs to the same clock domain
    pardsys.cpu = [TestCPUClass(clk_domain=pardsys.cpu_clk_domain, cpu_id=i)
                    for i in xrange(np)]

    if options.caches or options.l2cache:
        # By default the IOCache runs at the system clock
        pardsys.iocache = IOCache(addr_ranges = [AddrRange('3GB'), AddrRange(start='4GB', size='4GB')])
        pardsys.iocache.cpu_side = pardsys.iobus.master
        pardsys.iocache.mem_side = pardsys.membus.slave
    else:
        pardsys.iobridge = Bridge(delay='50ns', ranges = [AddrRange('3GB'), AddrRange(start='4GB', size='4GB')])
        pardsys.iobridge.slave = pardsys.iobus.master
        pardsys.iobridge.master = pardsys.membus.slave

    for i in xrange(np):
        pardsys.cpu[i].createThreads()

    CacheConfig.config_cache(options, pardsys)
    XMemConfig.config_mem(options, pardsys)

    return pardsys


# Add options
parser = optparse.OptionParser()
Options.addCommonOptions(parser)
Options.addFSOptions(parser)
(options, args) = parser.parse_args()
if args:
    print "Error: script doesn't take any positional arguments"
    sys.exit(1)

# system under test can be any CPU
(TestCPUClass, test_mem_mode, FutureClass) = XSimulation.setCPUClass(options)

# Match the memories with the CPUs, based on the options for the test system
TestMemClass = XSimulation.setMemClass(options)

bm = [SysConfig(disk=options.disk_image, mem=options.mem_size)]
np = options.num_cpus

pardsys = build_pardg5v_system(np)
root = Root(full_system=True, system=pardsys)

#### Build PRM system
prm = build_gm_system()
prm.cpn = CPNetwork()
prm.cpa = CPAdaptor(pci_bus=0, pci_dev=0x10, pci_func=0, InterruptLine=10,
                    InterruptPin=1);
prm.cpa.pio    = prm.iobus.master
prm.cpa.config = prm.iobus.master
prm.cpa.dma    = prm.iobus.slave
prm.cpa.master = prm.cpn.slave
root.prm = prm

pardsys.cp.connectToNetwork(prm.cpn)
pardsys.iobus.cp.connectToNetwork(prm.cpn)
pardsys.cellx.ich.cp.connectToNetwork(prm.cpn)

#### Change default UART port
prm.pc.com_1.terminal.port = 4456;
pardsys.cellx.ich.serials[0].terminal.port = 4456;
pardsys.cellx.ich.serials[1].terminal.port = 4456;
pardsys.cellx.ich.serials[2].terminal.port = 4456;
pardsys.cellx.ich.serials[3].terminal.port = 4456;
##############################################

XSimulation.setWorkCountOptions(pardsys, options)
XSimulation.run(options, root, pardsys, FutureClass)
