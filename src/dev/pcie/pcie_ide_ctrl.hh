/*
 * Copyright (c) 2004-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Andrew Schultz
 *          Miguel Serrano
 */

/** @file
 * Simple PCI IDE controller with bus mastering capability and UDMA
 * modeled after controller in the Intel PIIX4 chip
 */

#ifndef __PCIE_IDE_CTRL_HH__
#define __PCIE_IDE_CTRL_HH__

#include "dev/ide_ctrl.hh"
#include "params/PciEIdeController.hh"

/**
 * Device model for an Intel PIIX4 IDE controller, with MSI supported
 */

class PciEIdeController : public IdeController
{
  private:
    inline bool msiEnabled() { return (msicap.mc & 0x0001); }
    Tick readMSICapConfig(PacketPtr pkt);
    Tick writeMSICapConfig(PacketPtr pkt);

  public:
    typedef PciEIdeControllerParams Params;
    const Params *params() const { return (const Params *)_params; }
    PciEIdeController(Params *p);

    void intrPost();

    Tick writeConfig(PacketPtr pkt);
    Tick readConfig(PacketPtr pkt);
};
#endif // __PCIE_IDE_CTRL_HH_
