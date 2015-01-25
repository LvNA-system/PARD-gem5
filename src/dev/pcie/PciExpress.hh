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
 * Interface for devices using PCI-Express configuration
 */

#ifndef __DEV_PCIEXPRESS_HH__
#define __DEV_PCIEXPRESS_HH__

#include <cstring>
#include <vector>

#include "dev/pcidev.hh"
#include "mem/mport.hh"
#include "params/PciExpressDevice.hh"


/**
 * PCI-Express device, extend PCI config space with MSI-cap.
 */
class PciExpressDevice : public PciDevice
{
  protected:
    class MsiMasterPort : public MessageMasterPort
    {
        PciExpressDevice* device;
        Tick latency;
      public:
        MsiMasterPort(const std::string& _name, PciExpressDevice* dev,
                      Tick _latency) :
            MessageMasterPort(_name, dev), device(dev), latency(_latency)
        {
        }

        Tick recvResponse(PacketPtr pkt)
        {
            return 0;
            //return device->recvResponse(pkt);
        }

        //// This is x86 focused, so if this class becomes generic, this would
        //// need to be moved into a subclass.
        //void sendMessage(ApicList apics,
        //        TriggerIntMessage message, bool timing);
    };

  public:
    typedef PciExpressDeviceParams Params;
    const Params *
    params() const
    {
        return dynamic_cast<const Params *>(_params);
    }

  protected:
    MsiMasterPort msiPort;

    virtual Tick writeConfig(PacketPtr pkt);
    virtual Tick readConfig(PacketPtr pkt);

    inline bool msiEnabled() { return (msicap.mc & 0x0001); }
    void msiIntrPost();

  public:
    void
    intrPost()
    {
        if (!msiEnabled())
            platform->postPciInt(letoh(config.interruptLine));
        else
            msiIntrPost();
    }

    void
    intrClear()
    { if (!msiEnabled()) platform->clearPciInt(letoh(config.interruptLine)); }

  public:
    PciExpressDevice(const Params *params);

    virtual void init();

    virtual unsigned int drain(DrainManager *dm);

    virtual BaseMasterPort &getMasterPort(const std::string &if_name,
                                          PortID idx = InvalidPortID)
    {
        if (if_name == "msi") {
            return msiPort;
        }
        return PciDevice::getMasterPort(if_name, idx);
    }

};
#endif // __DEV_PCIEXPRESS_HH__
