from m5.params import *
from MemObject import MemObject
from Pci import PciDevice

class CPAdaptor(PciDevice):
    type = "CPAdaptor"
    cxx_header = "prm/CPAdaptor.hh"
    master = MasterPort("Master Ports")

    VendorID = 0x0A19
    DeviceID = 0x0001
    Command = 0x0
    Status = 0x0000
    Revision = 0x0
    ClassCode = 0x06		# Bridge Devices
    SubClassCode = 0x80		# Other bridge type
    ProgIF = 0x00
    BAR0 = 0x00000000		# CP selector register
    BAR1 = 0x00000000		# map to selected CP address space
    BAR0Size = '4B'
    BAR1Size = '32B'
    InterruptLine = 0x1a
    InterruptPin = 0x01
