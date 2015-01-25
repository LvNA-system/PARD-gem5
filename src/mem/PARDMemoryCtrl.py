from m5.params import *
from MemObject import MemObject
from XBar import CoherentXBar, NoncoherentXBar
from Bridge import Bridge

class PARDMemoryCtrl(MemObject):
    type = 'PARDMemoryCtrl'
    cxx_header = "mem/pard_mem_ctrl.hh"

    port = SlavePort("Slave port")

    # Internal DRAM Controller
    #  - TODO: Maybe we should use multiple DRAMCtrl?
    memories = Param.AbstractMemory("Internal memories")

    # Internal bus that connect all DRAMCtrl together
    _internal_bus = NoncoherentXBar()
    internal_bus = Param.NoncoherentXBar(_internal_bus, "Internal Bus")

    _internal_bridge = Bridge()
    internal_bridge = Param.Bridge(_internal_bridge, "Internal bridge")

    # Master port to access DRAMCtrl
    internal_port = MasterPort("Master port connect to internal bus")

    def attachDRAM(self):
        self.internal_port = self.internal_bridge.slave
        self.internal_bridge.master = self.internal_bus.slave
        #for mem in self.memories:
        #    self.internal_bus.master = mem.port
        self.internal_bus.master = self.memories.port

