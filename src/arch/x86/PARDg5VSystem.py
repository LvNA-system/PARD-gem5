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

from m5.params import *
from m5.proxy import *
from ControlPlane import ControlPlane
from System import System

# PARDg5VSystem Control Plane
class PARDg5VSystemCP(ControlPlane):
    type = 'PARDg5VSystemCP'
    cxx_header = 'arch/x86/pardg5v_system_cp.hh'

    # CPN address 0:0
    cp_dev = 0
    cp_fun = 0
    # Type 'S' System, IDENT: PARDg5VSysCP
    Type = 0x53
    IDENT = "PARDg5vSysCP"
    # Config memory size 16MB
    config_mem_size = MemorySize32('16MB')

    param_table_entries = Param.Int(32, "Number of parameter table entries")
    stat_table_entries  = Param.Int(32, "Number of statistics table entries")


class PARDg5VSystem(System):
    type = 'PARDg5VSystem'
    cxx_header = 'arch/x86/pardg5v_system.hh'

    # PARDg5VSystem Control Plane
    cp = Param.PARDg5VSystemCP(PARDg5VSystemCP(),
                               "Control plane for PARDg5VSystem")

    def connect(self, cpn):
        self.cp.connect(cpn)

