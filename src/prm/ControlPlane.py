from m5.params import *
from ClockedObject import ClockedObject
from CPConnector import CPConnector

class AbstractControlPlane(ClockedObject):
    type = 'AbstractControlPlane'
    abstract = True
    cxx_header = 'prm/AbstractControlPlane.hh'


class ControlPlane(AbstractControlPlane):
    type = 'ControlPlane'
    abstract = True
    cxx_header = 'prm/ControlPlane.hh'

    connector = Param.CPConnector("Connector to CPN")

    # Config memory are stored in BAR1
    config_mem_base = Param.Addr('0', "Config memory base addr")
    config_mem_size = Param.MemorySize32('0B', "Config memory size")

    # CPN-ADDRESS = (cp_dev<<8 | cp_fun)<<8
    cp_dev = Param.Int("ControlPanel device ID")        # 8-bit
    cp_fun = Param.Int("ControlPanel function ID")      # 8-bit -- UNUSED!!

    Type = Param.Int(0, "Type fo this CP")
    IDENT = Param.String("GenCP", "Identifier of this CP, 12-byte maximum")
    BAR0 = Param.UInt32(0x00, "Base Address Register 0")
    BAR0Size = Param.MemorySize32('0B', "Base Address Register 0 Size")
    BAR1 = Param.UInt32(0x00, "Base Address Register 0")
    BAR1Size = Param.MemorySize32('0B', "Base Address Register 0 Size")

    def connectToNetwork(self, cpn):
        self.connector = CPConnector(cp_dev = self.cp_dev,
                                     cp_fun = self.cp_fun,
                                     Type   = self.Type,
                                     IDENT  = self.IDENT,
                                     BAR0   = self.BAR0,
                                     BAR0Size = self.BAR0Size)
        self.connector.slave = cpn.master

    
class GeneralControlPlane(ControlPlane):
    type = 'GeneralControlPlane'
    cxx_header = 'prm/GeneralControlPlane.hh'

    # All tables are stored in BAR0
    parameter_attrs = VectorParam.String([], "Parameter table attributes")
    statistics_attrs = VectorParam.String([], "Statistics table attributes")
    trigger_entries_nr = Param.Int(0, "Trigger table size")

