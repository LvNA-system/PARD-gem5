/*
 * Copyright (c) 2014 Institute of Computing Technology, CAS
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
 * Authors: Jiuyue Ma
 */

#ifndef __MEM_PARD_PORT_PROXY_HH__
#define __MEM_PARD_PORT_PROXY_HH__

#include "mem/port_proxy.hh"

class PARDg5VPortProxy : public PortProxy
{
  private:

    /** The actual physical port used by this proxy. */
    MasterPort &_port;

    /** Granularity of any transactions issued through this proxy. */
    const unsigned int _cacheLineSize;

    /** DSid of current operation */
    uint16_t _DSid;

  public:
    PARDg5VPortProxy(MasterPort &port, unsigned int cacheLineSize) :
        PortProxy(port, cacheLineSize),
        _port(port), _cacheLineSize(cacheLineSize), _DSid(0xFFFF) { }
    virtual ~PARDg5VPortProxy() { }

    /**
     * Update current DSid.
     */
    void updateDSid(int DSid) { _DSid = DSid; }

    /**
     * Read size bytes memory at address and store in p.
     */
    virtual void readBlob(Addr addr, uint8_t* p, int size) const;

    /**
     * Write size bytes from p to address.
     */
    virtual void writeBlob(Addr addr, const uint8_t* p, int size) const;

};

#endif // __MEM_PARD_PORT_PROXY_HH__
