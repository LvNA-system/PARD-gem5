# Copyright (c) 2014 ACS, ICT.
# All rights reserved.
#
# Author: Jiuyue Ma

import optparse
import sys

from m5.objects import *
from m5.util import addToPath, fatal
addToPath('../../gem5-offical/configs/common')

import Simulation
import Options
from PARDg5GM import *


# Add options
parser = optparse.OptionParser()
Options.addCommonOptions(parser)
Options.addFSOptions(parser)
(options, args) = parser.parse_args()
if args:
    print "Error: script doesn't take any positional arguments"
    sys.exit(1)

#### Build PRM system
prm = build_gm_system()
prm.cpn = CPNetwork()
prm.cpa = CPAdaptor(pci_bus=0, pci_dev=0x10, pci_func=0, InterruptLine=10,
                    InterruptPin=1);
# Connect CPA to PRM iobus
prm.cpa.pio    = prm.iobus.master
prm.cpa.config = prm.iobus.master
prm.cpa.dma    = prm.iobus.slave
# Connect CPA to CPN
prm.cpa.master = prm.cpn.slave

#### Add test CPs
prm.cp0 = PARDg5VSystemCP()
prm.cp0.connectToNetwork(prm.cpn)

prm.cp1 = PARDg5VSystemCP(IDENT="HelloCP", cp_dev=10)
prm.cp1.connectToNetwork(prm.cpn)


root = Root(full_system=True, system=prm)
Simulation.run(options, root, prm, FutureClass)
