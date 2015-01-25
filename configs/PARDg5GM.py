# Copyright (c) 2014 ACS, ICT.
# All rights reserved.
#
# Author: Jiuyue Ma

import optparse
import sys

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.util import addToPath, fatal

addToPath('common')
addToPath('../../gem5-latest/configs/common')

from FSConfig import *
from SysPaths import *
from Benchmarks import *
import Simulation
import CacheConfig
import MemConfig
from Caches import *
import Options

class CPNetwork(NoncoherentXBar):
    fake_responder = IsaFake(pio_addr=0, pio_size=Addr('16MB'))
    default = Self.fake_responder.pio

#GlobalManager configurations
GMSysConfig = SysConfig(disk="PARDg5GM.img", mem="32MB")
GMBootOSFlags = 'earlyprintk=ttyS0 console=ttyS0 lpj=7999923 ' + \
                'root=/dev/sda1 no_timer_check init=/sbin/init'
GMKernel = binary('linux-2.6.28.4-prm/vmlinux')
GMCPUClass = AtomicSimpleCPU
FutureClass = None
gm_mem_mode = 'atomic'
GMMemClass = SimpleMemory

class GMMemory(SimpleMemory):
    range = AddrRange('32MB')

def build_gm_system():
    self = makeLinuxX86System(gm_mem_mode, 1, GMSysConfig)

    # IntelMPTable
    self.intel_mp_table.oem_id = "FSG"
    self.intel_mp_table.product_id = "PARDg5-V.r0"

    # Config kernel & boot flags
    self.boot_osflags = GMBootOSFlags
    self.kernel = GMKernel

    # Create voltage domain & source clock
    self.voltage_domain = VoltageDomain(voltage = '1.0V')
    self.cpu_voltage_domain = VoltageDomain()
    self.clk_domain = SrcClockDomain(voltage_domain = self.voltage_domain,
                                     clock = '1GHz')
    self.cpu_clk_domain = SrcClockDomain(voltage_domain = self.cpu_voltage_domain,
                                     clock = '100MHz')

    # For now, assign all the CPUs to the same clock domain
    self.cpu = GMCPUClass(clk_domain=self.cpu_clk_domain, cpu_id=0)
    self.cpu.createThreads()
    self.cpu.createInterruptController()
    self.cpu.connectAllPorts(self.membus)

    # Config Memory
    self.mem_ctrl = GMMemory()
    self.mem_ctrl.port = self.membus.master

    # Config IO Bridge
    self.iobridge = Bridge(delay='50ns', ranges = self.mem_ranges)
    self.iobridge.slave = self.iobus.master
    self.iobridge.master = self.membus.slave

    ####
    ##### Config Ethernet
    ####self.pc.eth0 = IGbE_e1000(pci_bus=0, pci_dev=2, pci_func=0,
    ####                          InterruptLine=13, InterruptPin=1)
    ####self.pc.eth0.pio = self.iobus.master
    ####self.pc.eth0.config = self.iobus.master
    ####self.pc.eth0.dma = self.iobus.slave

    ####self.ethertap = PARDg5VEtherTap(port=3500)
    ####self.etherlink = EtherLink()
    ####self.etherlink.int0 = self.pc.eth0.interface
    ####self.etherlink.int1 = self.ethertap.tap
    #####self.etherdump = EtherDump(file="eth0.dump")
    #####self.ethertap.dump = system.etherdump
    ####

    return self

