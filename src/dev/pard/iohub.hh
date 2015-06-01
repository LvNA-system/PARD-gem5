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

#ifndef __DEV_PARDG5V_IOHUB_HH__
#define __DEV_PARDG5V_IOHUB_HH__

#include <set>

#include "dev/pard/iohub_cp.hh"
#include "mem/noncoherent_xbar.hh"
#include "mem/tag_addr_mapper.hh"
#include "params/PARDg5VIOHub.hh"
#include "params/PARDg5VIOHubRemapper.hh"

struct PCI_CONFIG_SHADOW {
    uint32_t uniqBAR[5];
    uint32_t sizeBAR[5];
    uint32_t userBAR[5];
};

struct PCI_DEVICE {
    MemObject *owner;
    uint16_t pciid;
    uint8_t interruptLine;
    bool pard_compatible;
    std::vector<PortID> ports;
    struct PCI_CONFIG_SHADOW configShadow;
};


class PARDg5VIOHub : public NoncoherentXBar
{
    friend class PARDg5VIOHubRemapper;
    friend class PARDg5VIOHubCP;

  protected:

    class IOHubMasterPort : public NoncoherentXBarMasterPort
    {
      private:
        PARDg5VIOHub &ioh;

      public:
        IOHubMasterPort(const std::string &_name,
                        PARDg5VIOHub &_ioh, PortID _id)
            : NoncoherentXBarMasterPort(_name, _ioh, _id), ioh(_ioh)
        { }

      protected:
        virtual void recvRangeChange()
        {
            MemObject &slaveOwner = getSlavePort().getOwner();

            ioh.recvRangeChange(id);

            static const std::string system_devices_name[] = {
                ".cellx.behind_pci", ".cellx.pciconfig", ".apicbridge",
                ".iobridge", ".iocache", ".ich" };
            for (int i=0; i<6; i++) {
                if (slaveOwner.name().find(system_devices_name[i])
                      != std::string::npos)
                {
                    return;
                }
            }
            ioh.regDevicePort(slaveOwner, id);
        }
    };

    PARDg5VIOHubCP *cp;

  protected:
    // All PCI devices
    std::vector<struct PCI_DEVICE *> devices;

    // ConfigPort ==> uniqBAR[5]
    AddrRangeMap<struct PCI_CONFIG_SHADOW *> pciConfigShadow;
    // PioPort: <DSid, UserAddr> ==> uniqAddr
    std::map<uint16_t, AddrRangeMap<Addr> > pciIoShadow;

  protected:

    Addr remapAddr(Addr addr, uint16_t DSid);
    void hookPciAccess(PacketPtr pkt);

    struct PCI_DEVICE * getPciDevice(MemObject *owner)
    {
        for (auto dev : devices)
            if (dev->owner == owner)
                return dev;
        return NULL;
    }

    struct PCI_DEVICE * getPciDevice(int id)
    { return (id < devices.size()) ? devices[id] : NULL; }

    void regDevicePort(MemObject &owner, PortID port)
    {
        struct PCI_DEVICE *dev = new PCI_DEVICE;
        if ((dev = getPciDevice(&owner)) == NULL) {
            dev = new PCI_DEVICE;
            dev->owner = &owner;
            devices.push_back(dev);
        }
        dev->ports.push_back(port);
    }

  protected:

  public:
    PARDg5VIOHub(PARDg5VIOHubParams *p);
    virtual ~PARDg5VIOHub();

    virtual void startup();

};

class PARDg5VIOHubRemapper : public TagAddrMapper
{
    /**
     * IOHub this mapper belong to
     */
    PARDg5VIOHub *ioh;

  public:
    PARDg5VIOHubRemapper(const PARDg5VIOHubRemapperParams *p)
        : TagAddrMapper(p), ioh(p->ioh) { }
    virtual ~PARDg5VIOHubRemapper() { }

    virtual AddrRangeList getAddrRanges() const
    { return ioh->getAddrRanges(); }

  protected:
    virtual Addr remapAddr(Addr addr, uint16_t DSid) const
    { return ioh->remapAddr(addr, DSid); }
    virtual void preReqHook(PacketPtr pkt)
    { return ioh->hookPciAccess(pkt); }
    virtual void postRespHook(PacketPtr pkt)
    { return ioh->hookPciAccess(pkt); }
};

#endif	//__DEV_PARDG5V_IOHUB_HH__
