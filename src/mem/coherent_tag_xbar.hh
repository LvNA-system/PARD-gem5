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

#ifndef __MEM_COHERENT_TAG_XBAR_HH__
#define __MEM_COHERENT_TAG_XBAR_HH__

#include "mem/coherent_xbar.hh"
#include "params/CoherentTagXBar.hh"

/**
 * CoherentTagXBar is a coherent crossbar with used to assign DSid to packet
 * that pass through.
 */
class CoherentTagXBar : public CoherentXBar
{

  protected:

    /**
     * Declaration of the coherent crossbar slave port type, one will
     * be instantiated for each of the master ports connecting to the
     * crossbar.
     */
    class CoherentTagXBarSlavePort : public CoherentXBarSlavePort
    {

      private:

        /** A reference to the crossbar to which this port belongs. */
        CoherentTagXBar &xbar;

      public:

        CoherentTagXBarSlavePort(const std::string &_name,
                             CoherentTagXBar &_xbar, PortID _id)
            : CoherentXBarSlavePort(_name, _xbar, _id), xbar(_xbar)
        { }

      protected:

        /**
         * When receiving a timing request, pass it to the crossbar.
         */
        virtual bool recvTimingReq(PacketPtr pkt) {
            assert(!pkt->hasDSid());
            pkt->setDSid(xbar.DSid);
            return xbar.recvTimingReq(pkt, id);
        }

        /**
         * When receiving an atomic request, pass it to the crossbar.
         */
        virtual Tick recvAtomic(PacketPtr pkt) {
            //assert(!pkt->hasDSid());
            pkt->setDSid(xbar.DSid);
            return xbar.recvAtomic(pkt, id);
        }

        /**
         * When receiving a functional request, pass it to the crossbar.
         */
        virtual void recvFunctional(PacketPtr pkt) {
            assert(!pkt->hasDSid());
            pkt->setDSid(xbar.DSid);
            xbar.recvFunctional(pkt, id);
        }

    };

    /** DSid to be assigned */
    uint16_t DSid;

    /** DSid base address */
    Addr DSid_base_addr;        // TODO: check req addr, if in range, return DSid


  public:

    CoherentTagXBar(const CoherentTagXBarParams *p,
                   const unsigned int port_slave_connection_count);

    virtual ~CoherentTagXBar();

};

#endif //__MEM_COHERENT_TAG_XBAR_HH__
