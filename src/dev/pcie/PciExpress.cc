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

/* @file
 * A single PCI device configuration space entry.
 */

#include <list>
#include <string>
#include <vector>

#include "base/inifile.hh"
#include "base/intmath.hh"
#include "base/misc.hh"
#include "base/str.hh"
#include "base/trace.hh"
#include "debug/PCIDEV.hh"
#include "dev/alpha/tsunamireg.h"
#include "dev/pciconfigall.hh"
#include "dev/PciExpress.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "sim/byteswap.hh"
#include "sim/core.hh"


PciExpressDevice::PciExpressDevice(const Params *p)
    : PciDevice(p),
      msiPort(this->name() + ".msi_master", this, 0)
{
}

void
PciExpressDevice::init()
{
    if (!msiPort.isConnected())
        panic("PCI msi port on %s not connected to anything!\n", name());
   PciDevice::init();
}

unsigned int
PciExpressDevice::drain(DrainManager *dm)
{
    unsigned int count;
    count = pioPort.drain(dm) + dmaPort.drain(dm) + configPort.drain(dm)
          + msiPort.drain(dm);
    if (count)
        setDrainState(Drainable::Draining);
    else
        setDrainState(Drainable::Drained);
    return count;
}

Tick
PciExpressDevice::readConfig(PacketPtr pkt)
{
    int offset = pkt->getAddr() & PCI_CONFIG_SIZE;
    uint8_t *config_data = NULL;

    /* Access to MSI CAP */
    if (offset >= MSICAP_BASE &&
        offset < MSICAP_BASE+MSICAP_SIZE) {
        config_data = msicap.data;
        offset -= MSICAP_BASE;
    }
    else {
        return PciDevice::readConfig(pkt);
    }

    assert(config_data);

    switch (pkt->getSize()) {
      case sizeof(uint8_t):
        pkt->set<uint8_t>(config_data[offset]);
        DPRINTF(PCIDEV,
            "readConfig:  dev %#x func %#x reg %#x 1 bytes: data = %#x\n",
            params()->pci_dev, params()->pci_func, offset,
            (uint32_t)pkt->get<uint8_t>());
        break;
      case sizeof(uint16_t):
        pkt->set<uint16_t>(*(uint16_t*)&config_data[offset]);
        DPRINTF(PCIDEV,
            "readConfig:  dev %#x func %#x reg %#x 2 bytes: data = %#x\n",
            params()->pci_dev, params()->pci_func, offset,
            (uint32_t)pkt->get<uint16_t>());
        break;
      case sizeof(uint32_t):
        pkt->set<uint32_t>(*(uint32_t*)&config_data[offset]);
        DPRINTF(PCIDEV,
            "readConfig:  dev %#x func %#x reg %#x 4 bytes: data = %#x\n",
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
PciExpressDevice::writeConfig(PacketPtr pkt)
{
    int offset = pkt->getAddr() & PCI_CONFIG_SIZE;

    /* Access to MSI CAP */
    if (offset >= MSICAP_BASE && offset < MSICAP_BASE+MSICAP_SIZE) {
        switch (pkt->getSize()) {
          case sizeof(uint16_t):
            switch (offset - MSICAP_BASE) {
              case MSICAP_ID:
                msicap.mid = pkt->get<uint16_t>();
                break;
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
            DPRINTF(PCIDEV, "PCI write offset: %#x size: 2 data: %#x\n",
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
            DPRINTF(PCIDEV, "PCI write offset: %#x size: 4 data: %#x\n",
                    offset, (uint32_t)pkt->get<uint32_t>());
            break;
          default:
            panic("invalid access size(?) for PCI configspace!\n");
        }
    }
    else {
        return PciDevice::writeConfig(pkt);
    }

    pkt->makeAtomicResponse();
    return configDelay;
}

