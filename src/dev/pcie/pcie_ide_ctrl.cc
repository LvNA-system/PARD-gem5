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
 * Authors: Jiuyue Ma
 */

#include <string>

#include "cpu/intr_control.hh"
#include "debug/IdeCtrl.hh"
#include "debug/MSICAP.hh"
#include "dev/pcie_ide_ctrl.hh"
#include "dev/ide_disk.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "params/PciEIdeController.hh"
#include "sim/byteswap.hh"

// clang complains about std::set being overloaded with Packet::set if
// we open up the entire namespace std
using std::string;

PciEIdeController::PciEIdeController(Params *p)
    : IdeController(p)
{
}

void
PciEIdeController::intrPost()
{
    //primary.bmiRegs.status.intStatus = 1;
    if (!msiEnabled()) {
        //PciDevice::intrPost();
        IdeController::intrPost();
    }
    else {
        warn("PciEIdeController %s post MSI interrupt.\n", name());
        //PciDevice::intrPost();
        IdeController::intrPost();
    }
}

Tick
PciEIdeController::readConfig(PacketPtr pkt)
{
    int offset = pkt->getAddr() & PCI_CONFIG_SIZE;

    // read MSICAP config space
    if (offset >= MSICAP_BASE &&
        offset < MSICAP_BASE + MSICAP_SIZE)
    {
        return readMSICapConfig(pkt);
    }

    return IdeController::readConfig(pkt);
}


Tick
PciEIdeController::writeConfig(PacketPtr pkt)
{
    int offset = pkt->getAddr() & PCI_CONFIG_SIZE;

    // read MSICAP config space
    if (offset >= MSICAP_BASE &&
        offset < MSICAP_BASE + MSICAP_SIZE)
    {
        return writeMSICapConfig(pkt);
    }

    return IdeController::writeConfig(pkt);
}

Tick
PciEIdeController::readMSICapConfig(PacketPtr pkt)
{
    int offset = pkt->getAddr() & PCI_CONFIG_SIZE;

    /* Make sure access to MSICAP address range */
    assert (offset >= MSICAP_BASE &&
            offset <  MSICAP_BASE + MSICAP_SIZE);

    // PCI offset => MSICAP offset
    offset -= MSICAP_BASE;

    switch (pkt->getSize()) {
      case sizeof(uint8_t):
        pkt->set<uint8_t>(msicap.data[offset]);
        DPRINTF(MSICAP,
            "readMSICapConfig:  dev %#x func %#x reg %#x 1 bytes: data = %#x\n",
            params()->pci_dev, params()->pci_func, offset,
            (uint32_t)pkt->get<uint8_t>());
        break;
      case sizeof(uint16_t):
        pkt->set<uint16_t>(*(uint16_t*)&msicap.data[offset]);
        DPRINTF(MSICAP,
            "readMSICapConfig:  dev %#x func %#x reg %#x 2 bytes: data = %#x\n",
            params()->pci_dev, params()->pci_func, offset,
            (uint32_t)pkt->get<uint16_t>());
        break;
      case sizeof(uint32_t):
        pkt->set<uint32_t>(*(uint32_t*)&msicap.data[offset]);
        DPRINTF(MSICAP,
            "readMSICapConfig:  dev %#x func %#x reg %#x 4 bytes: data = %#x\n",
            params()->pci_dev, params()->pci_func, offset,
            (uint32_t)pkt->get<uint32_t>());
        break;
      default:
        panic("invalid access size(?) for PCI configspace!\n");
    }
    pkt->makeAtomicResponse();
    return configDelay;
}


Tick
PciEIdeController::writeMSICapConfig(PacketPtr pkt)
{
    int offset = pkt->getAddr() & PCI_CONFIG_SIZE;

    /* Make sure access to MSICAP address range */
    assert (offset >= MSICAP_BASE &&
            offset <  MSICAP_BASE + MSICAP_SIZE);

    /* Access to MSI CAP */
    switch (pkt->getSize()) {
      case sizeof(uint16_t):
        switch (offset - MSICAP_BASE) {
          case MSICAP_MC:
            msicap.mc = pkt->get<uint16_t>();
            break;
          case MSICAP_MD:
            msicap.md = pkt->get<uint16_t>();
            break;
          default:
            panic("Invalid PCI configuration write "
                    "for size 2 offset: %#x!\n",
                    offset);
        }
        DPRINTF(MSICAP, "PCI write offset: %#x size: 2 data: %#x\n",
                offset, (uint32_t)pkt->get<uint16_t>());
        break;
      case sizeof(uint32_t):
        switch (offset - MSICAP_BASE) {
          case MSICAP_MA:
            msicap.ma = pkt->get<uint32_t>();
            break;
          case MSICAP_MUA:
            msicap.mua = pkt->get<uint32_t>();
            break;
          case MSICAP_MMASK:
            msicap.mmask = pkt->get<uint32_t>();
            break;
          case MSICAP_MPEND:
            msicap.mpend = pkt->get<uint32_t>();
            break;
          default:
            panic("Invalid PCI configuration write "
                    "for size 4 offset: %#x!\n",
                    offset);
        }
        DPRINTF(MSICAP, "PCI write offset: %#x size: 4 data: %#x\n",
                offset, (uint32_t)pkt->get<uint32_t>());
        break;
      default:
        panic("invalid access size(?) for PCI configspace!\n");
    }

    pkt->makeAtomicResponse();
    return configDelay;
}

PciEIdeController *
PciEIdeControllerParams::create()
{
    return new PciEIdeController(this);
}
