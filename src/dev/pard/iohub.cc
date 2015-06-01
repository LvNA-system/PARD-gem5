/*
 * Copyright (c) 2015 Institute of Computing Technology, CAS
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

#include "arch/x86/x86_traits.hh"
#include "dev/pard/iohub.hh"
#include "dev/cellx/cellx.hh"
#include "dev/pcidev.hh"

using namespace X86ISA;


// Hack 1/3: hidden master/default port count to NoncoherentXBar
static int mcnt, dcnt;
PARDg5VIOHub::PARDg5VIOHub(PARDg5VIOHubParams *p)
    : // Hack 2/3: hidden master/default port count to NoncoherentXBar
      NoncoherentXBar((mcnt = p->port_master_connection_count,
                       dcnt = p->port_default_connection_count,
                       p->port_master_connection_count=0,
                       p->port_default_connection_count=0,
                       p)),
      cp(p->cp)
{
    // Hack 3/3: restore master/default port cont
    p->port_master_connection_count = mcnt;
    p->port_default_connection_count = dcnt;

    // create the ports based on the size of the master and slave
    // vector ports, and the presence of the default port, the ports
    // are enumerated starting from zero
    for (int i = 0; i < p->port_master_connection_count; ++i) {
        std::string portName = csprintf("%s.master[%d]", name(), i);
        MasterPort* bp = new IOHubMasterPort(portName, *this, i);
        masterPorts.push_back(bp);
        reqLayers.push_back(new ReqLayer(*bp, *this,
                                         csprintf(".reqLayer%d", i)));
    }

    // see if we have a default slave device connected and if so add
    // our corresponding master port
    if (p->port_default_connection_count) {
        defaultPortID = masterPorts.size();
        std::string portName = name() + ".default";
        MasterPort* bp = new IOHubMasterPort(portName, *this,
                                             defaultPortID);
        masterPorts.push_back(bp);
        reqLayers.push_back(new ReqLayer(*bp, *this, csprintf(".reqLayer%d",
                                                              defaultPortID)));
    }

    // register PARDg5VIOHCP
    cp->regPARDg5VIOHub(this);
}

PARDg5VIOHub::~PARDg5VIOHub()
{
    pciConfigShadow.clear();
    pciIoShadow.clear();
    for (auto dev : devices)
        delete dev;
    devices.clear();
}

static inline bool __isPciConfigPort(std::string name)
{
    static const char *pciConfigPortIdent = "-pciconf";
    static const int pciConfigPortIdentLen = strlen(pciConfigPortIdent);

    if (name.size() > pciConfigPortIdentLen &&
        name.compare(name.size() - pciConfigPortIdentLen,
                     strlen(pciConfigPortIdent),
                     pciConfigPortIdent) == 0)
        return true;
    return false;
}

void
PARDg5VIOHub::startup()
{
    NoncoherentXBar::startup();

    uint32_t basePciIOPort = 0x0000C000;
    uint32_t basePciIOMem  = 0xC1000000;
    pciConfigShadow.clear();
    pciIoShadow.clear();

    // Check each pci device's BARx size
    for (auto port : masterPorts) {
        if (!__isPciConfigPort(port->getSlavePort().name()))
            continue;
        assert(port->getAddrRanges().size() == 1);
        Addr configAddr = port->getAddrRanges().front().start();

        PCI_DEVICE *device = getPciDevice(&port->getSlavePort().getOwner());
        assert(device);

        device->pciid = CellX::calcPciID(configAddr);

        // Query InterruptLine of each PCI device
        {
            Request request(configAddr + PCI0_INTERRUPT_LINE,
                            4, Request::UNCACHEABLE,
                            Request::funcMasterId);
            Packet pkt(&request, MemCmd::ReadReq);
            pkt.allocate();
            port->sendFunctional(&pkt);
            device->interruptLine = pkt.get<uint8_t>();
        }

        // Build config shadow for each BAR
        PCI_CONFIG_SHADOW *shadow = &device->configShadow;
        memset(shadow, 0, sizeof(PCI_CONFIG_SHADOW));
        for (int i=0; i<5; i++) {
            Request request(configAddr + PCI0_BASE_ADDR0 + i*4,
                            4, Request::UNCACHEABLE,
                            Request::funcMasterId);
            Packet pkt(&request, MemCmd::WriteReq);
            pkt.allocate();
            // step1. query size
            pkt.set<uint32_t>(0xFFFFFFFF);
            port->sendFunctional(&pkt);
            pkt.cmd = MemCmd::ReadReq;
            port->sendFunctional(&pkt);

            int type = pkt.get<uint32_t>() & 0x1;
            // step2. record shadow address
            shadow->uniqBAR[i] = (type ? basePciIOPort : basePciIOMem) | type;
            shadow->sizeBAR[i] = ~(pkt.get<uint32_t>() & 0xFFFFFFFE) + 1;
            shadow->userBAR[i] = 0 | type;

            // step3. clearup our query
            pkt.cmd = MemCmd::WriteReq;
            pkt.set<uint32_t>(0);
            port->sendFunctional(&pkt);

            // step4. increate uniqBAR address
            (type ? basePciIOPort : basePciIOMem) += shadow->sizeBAR[i];
        }
        pciConfigShadow.insert(RangeSize(configAddr+PCI0_BASE_ADDR0, 20),
                               shadow);
    }

    cp->recvDeviceChange(devices);
}

#define calcPciConfigAddr(bus, dev, func) \
    (PhysAddrPrefixPciConfig | (func << 8) | (dev << 11))
Addr
PARDg5VIOHub::remapAddr(Addr addr, uint16_t DSid)
{
/*
    // ControlPlane do some remapping: logical==>physical
    // (e.g. 0xA0000000xxxxxxxx)
    Addr base, remapped;
    if (cp->remapAddr(DSid, addr, &base, &remapped))
        addr = remapped + (addr-base);
*/

    // Check device mask, hidden non-assigned devices
    if ((addr & 0xF000000000000000) == PhysAddrPrefixPciConfig) {
        // XXX: Make 0000:31:7 INVALID device
        static const Addr InvalidPciConfigAddress = calcPciConfigAddr(0, 31, 7);

        // get device mask
        uint32_t device_mask = cp->getDeviceMask(DSid);

        // check device mask
        for (int i=0; i<32 && device_mask!=0; i++) {
            if ((device_mask & (1<<i)) &&
                (devices[i]->pciid == CellX::calcPciID(addr)))
                return addr;
        }
 
        return InvalidPciConfigAddress;
    }
    else {
        // Remapping PCI I/O address (e.g. 0x80000000xxxxxxxx)
        auto p = pciIoShadow[DSid].find(addr);
        if (p != pciIoShadow[DSid].end())
            addr += p->second;
    }

    return addr;
}

void
PARDg5VIOHub::hookPciAccess(PacketPtr pkt)
{
    // if access to PCI configure space
    MasterPort *port = masterPorts[findPort(pkt->getAddr())];
    if (__isPciConfigPort(port->getSlavePort().name())) {
        int offset = pkt->getAddr() & PCI_CONFIG_SIZE;

        // access to BAR register
        if (offset >= PCI0_BASE_ADDR0 &&
            offset <= PCI0_BASE_ADDR4)
        {
            int barnum = BAR_NUMBER(offset);
            struct PCI_CONFIG_SHADOW *shadow =
                               pciConfigShadow.find(pkt->getAddr())->second;

            // for write request to BAR register, we shadow it
            if (pkt->isRequest() && pkt->isWrite()) {
                uint32_t base = pkt->get<uint32_t>() & 0xFFFFFFFE;
                uint32_t type = pkt->get<uint32_t>() & 0x00000001;
                if (base == 0xFFFFFFFE || base == 0)
                    return;

                shadow->userBAR[barnum] = pkt->get<uint32_t>();
                pciIoShadow[pkt->getDSid()].insert(
                    RangeSize((type ? x86IOAddress(shadow->userBAR[barnum])
                                    : shadow->userBAR[barnum]) & 0xFFFFFFFFFFFFFFFE,
                               shadow->sizeBAR[barnum]),
                    shadow->uniqBAR[barnum]-shadow->userBAR[barnum]);
                pkt->set<uint32_t>(shadow->uniqBAR[barnum]);
            }
            // for read response, check shadow and reture original user BAR
            else if (pkt->isResponse() && pkt->isRead()) {
                if (pkt->get<uint32_t>() == shadow->uniqBAR[barnum])
                    pkt->set<uint32_t>(shadow->userBAR[barnum]);
            }
/* Let them go!
            // Oops, we are not expect other packet type
            else {
                panic("PARDg5VIOHub don't know how to deal with this packet:\n%s",
                      pkt->print().c_str());
            }
*/
        }
    }
}

PARDg5VIOHubRemapper *
PARDg5VIOHubRemapperParams::create()
{
    return new PARDg5VIOHubRemapper(this);
}

PARDg5VIOHub *
PARDg5VIOHubParams::create()
{
    return new PARDg5VIOHub(this);
}
