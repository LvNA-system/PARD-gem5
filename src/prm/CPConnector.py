from m5.params import *
from MemObject import MemObject

class CPConnector(MemObject):
    type = "CPConnector"
    cxx_header = "prm/CPConnector.hh"
    slave  = SlavePort("Slave Ports")

    # CPN-ADDRESS = (cp_dev<<8 | cp_fun)<<256
    cp_dev = Param.Int("ControlPanel device ID")	# 8-bit
    cp_fun = Param.Int("ControlPanel function ID")	# 8-bit

    Type = Param.Int(0, "Type fo this CP")
    IDENT = Param.String("GenCP", "Identifier of this CP, 12-byte maximum")
    BAR0 = Param.UInt32(0x00, "Base Address Register 0")
    BAR0Size = Param.MemorySize32('0B', "Base Address Register 0 Size")
