/*
 * Copyright (c) 2014 Institute of Computing Technology, CAS
 * Copyright (c) 2012 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
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
 * Authors: Andreas Hansson
 *          Jiuyue Ma
 */

#ifndef __MEM_TAG_ADDR_MAPPER_HH__
#define __MEM_TAG_ADDR_MAPPER_HH__

#include "mem/mem_object.hh"
#include "params/TagAddrMapper.hh"

/**
 * majiuyue:
 * Add extra "tag" parameter for remapAddr() function, used by PARDg5-V
 */

/**
 * An address mapper changes the packet addresses in going from the
 * slave port side of the mapper to the master port side. When the
 * slave port is queried for the address ranges, it also performs the
 * necessary range updates. Note that snoop requests that travel from
 * the master port (i.e. the memory side) to the slave port are
 * currently not modified.
 */

class TagAddrMapper : public MemObject
{

  public:

    TagAddrMapper(const TagAddrMapperParams* params);

    virtual ~TagAddrMapper() { }

    virtual BaseMasterPort& getMasterPort(const std::string& if_name,
                                          PortID idx = InvalidPortID);

    virtual BaseSlavePort& getSlavePort(const std::string& if_name,
                                        PortID idx = InvalidPortID);

    virtual void init();

  protected:

    /**
     * This function does the actual remapping of one address to another.
     * It is pure virtual in this case to to allow any implementation
     * required.
     * @param addr the address to remap
     * @return the new address (can be unchanged)
     */
    virtual Addr remapAddr(Addr addr, uint16_t DSid) const = 0;

    class TagAddrMapperSenderState : public Packet::SenderState
    {

      public:

        /**
         * Construct a new sender state to remember the original address.
         *
         * @param _origAddr Address before remapping
         */
        TagAddrMapperSenderState(Addr _origAddr) : origAddr(_origAddr)
        { }

        /** Destructor */
        ~TagAddrMapperSenderState() { }

        /** The original address the packet was destined for */
        Addr origAddr;

    };

    class MapperMasterPort : public MasterPort
    {

      public:

        MapperMasterPort(const std::string& _name, TagAddrMapper& _mapper)
            : MasterPort(_name, &_mapper), mapper(_mapper)
        { }

      protected:

        void recvFunctionalSnoop(PacketPtr pkt)
        {
            mapper.recvFunctionalSnoop(pkt);
        }

        Tick recvAtomicSnoop(PacketPtr pkt)
        {
            return mapper.recvAtomicSnoop(pkt);
        }

        bool recvTimingResp(PacketPtr pkt)
        {
            return mapper.recvTimingResp(pkt);
        }

        void recvTimingSnoopReq(PacketPtr pkt)
        {
            mapper.recvTimingSnoopReq(pkt);
        }

        void recvRangeChange()
        {
            mapper.recvRangeChange();
        }

        bool isSnooping() const
        {
            return mapper.isSnooping();
        }

        void recvRetry()
        {
            mapper.recvRetryMaster();
        }

      private:

        TagAddrMapper& mapper;

    };

    /** Instance of master port, facing the memory side */
    MapperMasterPort masterPort;

    class MapperSlavePort : public SlavePort
    {

      public:

        MapperSlavePort(const std::string& _name, TagAddrMapper& _mapper)
            : SlavePort(_name, &_mapper), mapper(_mapper)
        { }

      protected:

        void recvFunctional(PacketPtr pkt)
        {
            mapper.recvFunctional(pkt);
        }

        Tick recvAtomic(PacketPtr pkt)
        {
            return mapper.recvAtomic(pkt);
        }

        bool recvTimingReq(PacketPtr pkt)
        {
            return mapper.recvTimingReq(pkt);
        }

        bool recvTimingSnoopResp(PacketPtr pkt)
        {
            return mapper.recvTimingSnoopResp(pkt);
        }

        AddrRangeList getAddrRanges() const
        {
            return mapper.getAddrRanges();
        }

        void recvRetry()
        {
            mapper.recvRetrySlave();
        }

      private:

        TagAddrMapper& mapper;

    };

    /** Instance of slave port, i.e. on the CPU side */
    MapperSlavePort slavePort;

    void recvFunctional(PacketPtr pkt);

    void recvFunctionalSnoop(PacketPtr pkt);

    Tick recvAtomic(PacketPtr pkt);

    Tick recvAtomicSnoop(PacketPtr pkt);

    bool recvTimingReq(PacketPtr pkt);

    bool recvTimingResp(PacketPtr pkt);

    void recvTimingSnoopReq(PacketPtr pkt);

    bool recvTimingSnoopResp(PacketPtr pkt);

    virtual AddrRangeList getAddrRanges() const = 0;

    bool isSnooping() const;

    void recvRetryMaster();

    void recvRetrySlave();

    void recvRangeChange();
};

#endif //__MEM_TAG_ADDR_MAPPER_HH__
