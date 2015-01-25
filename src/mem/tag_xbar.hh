/*
 * Copyright (c) 2014 Institute of Computing Technology, CAS
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

/**
 * @file
 * Declaration of coherent PARD system crossbar.
 */

#ifndef __MEM_TAG_XBAR_HH__
#define __MEM_TAG_XBAR_HH__

#include "mem/coherent_xbar.hh"
#include "params/TagXBar.hh"

/**
 * CoherentTagXBar is a coherent crossbar with used to assign DSid to packet
 * that pass through.
 */
class TagXBar : public MemObject
{
  protected:

    class RequestState : public Packet::SenderState
    {
      public:
        const PortID origSrc;
        RequestState(PortID orig_src) : origSrc(orig_src)
        { }
    };


    class TagXBarMasterPort;

    class TagXBarSlavePort : public SlavePort
    {
      private:

        /** A reference to the crossbar to which this port belongs. */
        TagXBar &xbar;
        TagXBarMasterPort &masterPort;

      public:

        TagXBarSlavePort(const std::string &_name,
                         TagXBar &_xbar, PortID _id, 
                         TagXBarMasterPort &_masterPort)
            : SlavePort(_name, &_xbar, _id), xbar(_xbar),
              masterPort(_masterPort)
        { }

      protected:

        virtual bool recvTimingReq(PacketPtr pkt) {
            pkt->firstWordDelay = pkt->lastWordDelay = 0;
            return xbar.recvTimingReq(pkt);
        }

        virtual bool recvTimingSnoopResp(PacketPtr pkt) {
            pkt->firstWordDelay = pkt->lastWordDelay = 0;
            return xbar.recvTimingSnoopResp(pkt);
        }

        virtual Tick recvAtomic(PacketPtr pkt) {
            pkt->firstWordDelay = pkt->lastWordDelay = 0;
            return masterPort.sendAtomic(pkt);
        }

        virtual void recvFunctional(PacketPtr pkt) {
            pkt->firstWordDelay = pkt->lastWordDelay = 0;
            masterPort.sendFunctional(pkt);
        }

        virtual void recvRetry()
        { panic("Crossbar slave ports should never retry.\n"); }

        virtual AddrRangeList getAddrRanges() const
        { return masterPort.getAddrRanges(); }
    };

    class TagXBarMasterPort : public MasterPort
    {
      private:
        TagXBar &xbar;
        TagXBarSlavePort &slavePort;

      public:
        TagXBarMasterPort(const std::string &_name,
                          TagXBar &_xbar, PortID _id, 
                          TagXBarSlavePort &_slavePort)
            : MasterPort(_name, &_xbar, _id), xbar(_xbar),
              slavePort(_slavePort)
        { }

      protected:

        virtual bool isSnooping() const
        { return true; }

        virtual bool recvTimingResp(PacketPtr pkt) {
            pkt->firstWordDelay = pkt->lastWordDelay = 0;
            return xbar.recvTimingResp(pkt);
        }

        virtual void recvTimingSnoopReq(PacketPtr pkt) {
            pkt->firstWordDelay = pkt->lastWordDelay = 0;
            return slavePort.sendTimingSnoopReq(pkt);
        }

        virtual Tick recvAtomicSnoop(PacketPtr pkt) {
            pkt->firstWordDelay = pkt->lastWordDelay = 0;
            return slavePort.sendAtomicSnoop(pkt);
        }

        virtual void recvFunctionalSnoop(PacketPtr pkt) {
            pkt->firstWordDelay = pkt->lastWordDelay = 0;
            slavePort.sendFunctionalSnoop(pkt);
        }

        virtual void recvRangeChange()
        { slavePort.sendRangeChange(); }

        virtual void recvRetry()
        { slavePort.sendRetry(); }

    };

  protected:

    /** DSid to be assigned */
    uint16_t DSid;

    /** DSid base address */
    Addr DSid_base_addr;        // TODO: check req addr, if in range, return DSid

    /** Internal XBar */
    CoherentXBar xbar;

    /** Internal Port */
    TagXBarSlavePort slavePort;
    TagXBarMasterPort masterPort;

  public:

    TagXBar(const TagXBarParams *p);
    virtual ~TagXBar();

    virtual void init() { xbar.init(); }
    virtual unsigned int drain(DrainManager *dm) { return xbar.drain(dm); }
    virtual void regStats() { xbar.regStats(); }

  public:

    /** A function used to return the port associated with this object. */
    BaseMasterPort& getMasterPort(const std::string& if_name,
                                  PortID idx = InvalidPortID) {
        if (if_name == "master")
            return masterPort;
        else
            return MemObject::getMasterPort(if_name, idx);
    }
    BaseSlavePort& getSlavePort(const std::string& if_name,
                                PortID idx = InvalidPortID) {
        return xbar.getSlavePort(if_name, idx);
    }

  protected:
    bool recvTimingReq(PacketPtr pkt);
    bool recvTimingResp(PacketPtr pkt);
    void recvTimingSnoopReq(PacketPtr pkt);
    bool recvTimingSnoopResp(PacketPtr pkt);
};

#endif //__MEM_TAG_XBAR_HH__
