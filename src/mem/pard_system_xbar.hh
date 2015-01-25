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

#ifndef __MEM_PARD_SYSTEM_XBAR_HH__
#define __MEM_PARD_SYSTEM_XBAR_HH__

#include "mem/coherent_xbar.hh"
#include "params/PARDSystemXBar.hh"

/**
 * PARD system crossbar is a coherent crossbar with two extra ports 
 * which connect to memory and I/O bridge. Address ranges that pass
 * through to those ports are fixed at configure.
 *  - e.g. 0-3G for memory port, and PCI/IO for I/O port
 */
class PARDSystemXBar : public CoherentXBar
{

  protected:

    /**
     * Declaration of the coherent crossbar slave port type, one will
     * be instantiated for each of the master ports connecting to the
     * crossbar.
     */
    class PARDSystemXBarSlavePort : public CoherentXBarSlavePort
    {

      private:

        /** A reference to the crossbar to which this port belongs. */
        PARDSystemXBar &xbar;

      public:

        PARDSystemXBarSlavePort(const std::string &_name,
                             PARDSystemXBar &_xbar, PortID _id)
            : CoherentXBarSlavePort(_name, _xbar, _id), xbar(_xbar)
        { }

      protected:

        /**
         * When receiving a timing request, pass it to the crossbar.
         */
        virtual bool recvTimingReq(PacketPtr pkt)
        { return xbar.recvTimingReq(pkt, id); }

        /**
         * When receiving an atomic request, pass it to the crossbar.
         */
        virtual Tick recvAtomic(PacketPtr pkt)
        { return xbar.recvAtomic(pkt, id); }

        /**
         * When receiving a functional request, pass it to the crossbar.
         */
        virtual void recvFunctional(PacketPtr pkt)
        { xbar.recvFunctional(pkt, id); }

        /**
         * Return the union of all adress ranges seen by this crossbar.
         */
        virtual AddrRangeList getAddrRanges() const
        { return xbar.getAddrRanges(); }

    };

    /**
     * Declaration of the coherent crossbar master port type, one will be
     * instantiated for each of the slave interfaces connecting to the
     * crossbar.
     */
    class PARDSystemXBarMasterPort : public CoherentXBarMasterPort
    {
      private:
        /** A reference to the crossbar to which this port belongs. */
        PARDSystemXBar &xbar;

      public:

        PARDSystemXBarMasterPort(const std::string &_name,
                                 PARDSystemXBar &_xbar, PortID _id)
            : CoherentXBarMasterPort(_name, _xbar, _id), xbar(_xbar)
        { }

      protected:

        /** 
         * When receiving a timing snoop request, pass it to the crossbar.
         */
        virtual void recvTimingSnoopReq(PacketPtr pkt)
        { return xbar.recvTimingSnoopReq(pkt, id); }

        /** When reciving a range change from the peer port, do nothing.
            Because we have fixed address range for this master port. */
        virtual void recvRangeChange() {
            // TODO: check if new range in fixed address range
        }

    };


    /** Fixed PortMap for memoryPort and ioPort */
    AddrRangeMap<PortID> fixedPortMap;

    /** Memory port */
    MasterPort *memoryPort;
    PortID memoryPortID;
    std::vector<AddrRange> memoryRanges;

    /** I/O Bridge port */
    MasterPort *ioPort;
    PortID ioPortID;
    std::vector<AddrRange> ioRanges;


  protected:

    /** Function called by the port when the crossbar is recieving a Timing
      request packet.*/
    bool recvTimingReq(PacketPtr pkt, PortID slave_port_id);

    /** Function called by the port when the crossbar is recieving a timing
        snoop request.*/
    void recvTimingSnoopReq(PacketPtr pkt, PortID master_port_id);

    /** Function called by the port when the crossbar is recieving a Atomic
      transaction.*/
    Tick recvAtomic(PacketPtr pkt, PortID slave_port_id);

    /** Function called by the port when the crossbar is recieving a Functional
        transaction.*/
    void recvFunctional(PacketPtr pkt, PortID slave_port_id);
 
    /**
     * Return the address ranges the crossbar is responsible for.
     *
     * @return a list of non-overlapping address ranges
     *         PARD extras: add memory/io port range to return list
     */
    AddrRangeList getAddrRanges() const;

  public:

    /** A function used to return the port associated with this object. */
    virtual BaseMasterPort& getMasterPort(const std::string& if_name,
                                          PortID idx = InvalidPortID);

  public:

    PARDSystemXBar(const PARDSystemXBarParams *p,
                   const unsigned int port_slave_connection_count);

    virtual ~PARDSystemXBar();

};

#endif //__MEM_PARD_SYSTEM_XBAR_HH__
