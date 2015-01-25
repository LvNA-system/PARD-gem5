from m5.params import *
from XBridge import XBridge

class RootComplex(XBridge):
    type = 'RootComplex'
    cxx_header = "dev/pcie/root_complex.hh"

