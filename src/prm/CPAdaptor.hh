/*
 * Copyright (c) 2014 ACS, ICT
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

#ifndef __HYPER_GM_CPADAPTOR_HH__
#define __HYPER_GM_CPADAPTOR_HH__

#include "dev/pcidev.hh"
#include "mem/mport.hh"
#include "mem/packet.hh"
#include "params/CPAdaptor.hh"

class CPAdaptor : public PciDevice
{
  protected:

    class CPAdaptorMasterPort : public MessageMasterPort
    {
      protected:
        CPAdaptor *agent;
      
      public:
        CPAdaptorMasterPort(const std::string& _name, CPAdaptor *_agent)
          : MessageMasterPort(_name, static_cast<MemObject *>(_agent)), agent(_agent)
        {}

        Tick recvResponse(PacketPtr pkt) { return agent->recvResponse(pkt); }
    };

    /** Master ports of the bridge. */
    CPAdaptorMasterPort masterPort;

  protected:

    BitUnion32(CPACommandReg)
        Bitfield<9, 0> selectCP;
    EndBitUnion(CPACommandReg)

    struct CPARegs {
        CPACommandReg command;
    } cpaRegs;

/*
    struct CPCRegs {
        uint32_t cpType;
        uint32_t cpIdentLow;
        uint64_t cpIdentHigh;
        uint8_t  command;
        uint8_t  state;
        uint16_t LDomID;
        uint32_t destAddr;
        uint64_t data;
    } cpcRegs;
*/

   

    // access dispatcher
    void dispatchAccess(PacketPtr pkt, bool read);

    // method to access cpaRegs
    void accessCommand(Addr offset, int size, uint8_t *data, bool read);
    // method to access selected CPC's register space
    void accessData(Addr offset, int size, uint8_t *data, bool read);

  public:

    typedef CPAdaptorParams Params;
    CPAdaptor(Params *p);

    virtual void init();

    virtual BaseMasterPort& getMasterPort(const std::string &if_name, PortID idx) {
        return (if_name == "master") ? masterPort
                                     : PciDevice::getMasterPort(if_name, idx);
    }

  /**
   * PCI Interface
   **/
  public:

    virtual Tick  read(PacketPtr pkt) { dispatchAccess(pkt, true); return pioDelay; }
    virtual Tick write(PacketPtr pkt) { dispatchAccess(pkt, false); return pioDelay; }


  /**
   * CPN Interface
   **/
  public:

    Tick recvResponse(PacketPtr pkt);

};

#define CPA_COMMAND_OFFSET	(0)

#endif //__HYPER_GM_CPADAPTOR_HH__
