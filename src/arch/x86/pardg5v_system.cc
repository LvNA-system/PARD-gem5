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

#include "arch/x86/pardg5v_system.hh"
#include "arch/x86/pard_interrupts.hh"
#include "arch/x86/pard_tlb.hh"
#include "base/loader/object_file.hh"
#include "base/loader/symtab.hh"
#include "cpu/thread_context.hh"
#include "cpu/base.hh"
#include "debug/Loader.hh"
#include "debug/PARDg5VSystem.hh"
#include "params/PARDg5VSystem.hh"

using namespace X86ISA;


PARDg5VSystem::PARDg5VSystem(Params *p)
    : System(p), cp(p->cp), 
      physProxy(this->getSystemPort(), this->cacheLineSize())
{
    cp->registerCommandHandler(static_cast<ICommandHandler *>(this));
}

PARDg5VSystem::~PARDg5VSystem()
{
}

void
PARDg5VSystem::initState()
{
    //copied from System::initState()
    activeCpus.clear();

    // Stop all thread contexts
    for (int i=0; i<threadContexts.size(); i++)
        threadContexts[i]->suspend();
}

bool
PARDg5VSystem::handleCommand(int cmd, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    uint16_t DSid = (uint16_t)arg1;

    DPRINTF(PARDg5VSystem, "handleCommand(cmd=%d, DSid=%d)\n", cmd, DSid);

    // Check PARDg5VSystem commands
    if (cmd == 'B') {		// bootup ldom
        startupLDomain(DSid);
    } else if (cmd == 'K') {	// kill ldom
        killLDomain(DSid);
    } else {
        return false;
    }

    return true;
}

void
PARDg5VSystem::startupLDomain(uint16_t DSid)
{
    std::vector<ThreadContext *> tc_list;
    ThreadContext *tcBSP = NULL;

    DPRINTF(PARDg5VSystem, "Starting up LDomain %X...\n", DSid);

    // Get available thread contexts, 64-max
    uint64_t cpuMask;
    if (cp->getCpuMask(DSid, &cpuMask) < 0) {
        warn("Try to startup unknown system with DSid: %d\n", DSid);
        return;
    }
    for (int i=0; i<threadContexts.size(); i++) {
        if (1<<i & cpuMask)
            tc_list.push_back(threadContexts[i]);
    }
    panic_if(tc_list.size() != 1,
             "PARDg5-V onlys support 1-core per domain. CpuMask: 0x%x\n", cpuMask);

    // Update DSid in ThreadContext packet source (e.g. itb/dtb, localApic, QR0?)
    for (auto tc : tc_list) {
        /* Update DSid in itb/dtb */
        PardTLB *itb = dynamic_cast<PardTLB *>(tc->getITBPtr());
        PardTLB *dtb = dynamic_cast<PardTLB *>(tc->getDTBPtr());
        assert(itb && dtb);
        itb->updateDSid(DSid);
        dtb->updateDSid(DSid);

        /* Update DSid in localApic */
        PardInterrupts *localApic = dynamic_cast<PardInterrupts *>(
            tc->getCpuPtr()->getInterruptController());
        assert(localApic);
        localApic->updateDSid(DSid);

        /** 
         * TODO: do we really needs MISCREG_QR0?
         *****
        tc->setMiscReg(MISCREG_QR0, (MiscReg)DSid); */
    }
    physProxy.updateDSid(DSid);

    // Load linux kernel to ldom's memory, SIMULATION-ONLY!!
    // In real system, kernel will be parsed by PRM and placed in config
    // table as other config data (e.g. SMBios, E820, ACPI)
    if (!kernel)
        fatal("No kernel to load.\n");
    if (kernel->getArch() == ObjectFile::I386)
        fatal("Loading a 32 bit x86 kernel is not supported.\n");
    DPRINTF(PARDg5VSystem, "LDomain#%X: Loading kernel...\n", DSid);
    kernel->loadSections(physProxy, loadAddrMask, loadAddrOffset);
    DPRINTF(Loader, "Kernel start = %#x\n", kernelStart);
    DPRINTF(Loader, "Kernel end   = %#x\n", kernelEnd);
    DPRINTF(Loader, "Kernel entry = %#x\n", kernelEntry);
    DPRINTF(Loader, "Kernel loaded...\n");

   /*
    * Deal with the command line stuff. SIMULATION-ONLY!!
    */

    // A buffer to store the command line.
    std::string commandLine = params()->boot_osflags;
    const Addr realModeData = 0x90200; 
    const Addr commandLineBuff = 0x90000;
    // A pointer to the commandLineBuff stored in the real mode data.
    const Addr commandLinePointer = realModeData + 0x228;

    if (commandLine.length() + 1 > realModeData - commandLineBuff)
        panic("Command line \"%s\" is longer than %d characters.\n",
                commandLine, realModeData - commandLineBuff - 1);
    physProxy.writeBlob(commandLineBuff, (uint8_t *)commandLine.c_str(),
                        commandLine.length() + 1);

    // Generate a pointer of the right size and endianness to put into
    // commandLinePointer.
    uint32_t guestCommandLineBuff =
        X86ISA::htog((uint32_t)commandLineBuff);
    physProxy.writeBlob(commandLinePointer, (uint8_t *)&guestCommandLineBuff,
                        sizeof(guestCommandLineBuff));
 

    // select one thread context as BSP, and initialize it
    DPRINTF(PARDg5VSystem, "LDomain#%X: Initialize BSP state...\n", DSid);
    tcBSP = tc_list[0];
    initBSPState(DSid, tcBSP);

    // kickstart BSP
    tcBSP->activate();
    DPRINTF(PARDg5VSystem, "LDomain 0x%x startup OK\n", DSid);
}

void
PARDg5VSystem::killLDomain(uint16_t DSid)
{
    // TODO: kill ldoms
    warn("Ignore unimpl kill logical domain.\n");
}

/*
static void
installSegDesc(ThreadContext *tc, SegmentRegIndex seg,
               SegDescriptor desc, bool longmode)
{
    uint64_t base = desc.baseLow + (desc.baseHigh << 24);
    bool honorBase = !longmode || seg == SEGMENT_REG_FS ||
                                  seg == SEGMENT_REG_GS ||
                                  seg == SEGMENT_REG_TSL ||
                                  seg == SYS_SEGMENT_REG_TR;
    uint64_t limit = desc.limitLow | (desc.limitHigh << 16);

    SegAttr attr = 0;

    attr.dpl = desc.dpl;
    attr.unusable = 0;
    attr.defaultSize = desc.d;
    attr.longMode = desc.l;
    attr.avl = desc.avl;
    attr.granularity = desc.g;
    attr.present = desc.p;
    attr.system = desc.s;
    attr.type = desc.type;
    if (desc.s) {
        if (desc.type.codeOrData) {
            // Code segment
            attr.expandDown = 0;
            attr.readable = desc.type.r;
            attr.writable = 0;
        } else {
            // Data segment
            attr.expandDown = desc.type.e;
            attr.readable = 1;
            attr.writable = desc.type.w;
        }
    } else {
        attr.readable = 1;
        attr.writable = 1;
        attr.expandDown = 0;
    }

    tc->setMiscReg(MISCREG_SEG_BASE(seg), base);
    tc->setMiscReg(MISCREG_SEG_EFF_BASE(seg), honorBase ? base : 0);
    tc->setMiscReg(MISCREG_SEG_LIMIT(seg), limit);
    tc->setMiscReg(MISCREG_SEG_ATTR(seg), (MiscReg)attr);
} 
*/

void
PARDg5VSystem::initBSPState(uint16_t DSid, ThreadContext *tcBSP)
{
    ThreadContext *tc = tcBSP;

    // This is the boot strap processor (BSP). Initialize it to look like
    // the boot loader has just turned control over to the 64 bit OS. We
    // won't actually set up real mode or legacy protected mode descriptor
    // tables because we aren't executing any code that would require
    // them. We do, however toggle the control bits in the correct order
    // while allowing consistency checks and the underlying mechansims
    // just to be safe.

    const int NumPDTs = 4;

    const Addr PageMapLevel4 = 0x70000;
    const Addr PageDirPtrTable = 0x71000;
    const Addr PageDirTable[NumPDTs] =
        {0x72000, 0x73000, 0x74000, 0x75000};
    const Addr GDTBase = 0x76000;

    const int PML4Bits = 9;
    const int PDPTBits = 9;
    const int PDTBits = 9;

    /*
     * Set up the gdt.
     */
    uint8_t numGDTEntries = 0;
    // Place holder at selector 0
    uint64_t nullDescriptor = 0;
    physProxy.writeBlob(GDTBase + numGDTEntries * 8,
                        (uint8_t *)(&nullDescriptor), 8);
    numGDTEntries++;

    SegDescriptor initDesc = 0;
    initDesc.type.codeOrData = 0; // code or data type
    initDesc.type.c = 0;          // conforming
    initDesc.type.r = 1;          // readable
    initDesc.dpl = 0;             // privilege
    initDesc.p = 1;               // present
    initDesc.l = 1;               // longmode - 64 bit
    initDesc.d = 0;               // operand size
    initDesc.g = 1;               // granularity
    initDesc.s = 1;               // system segment
    initDesc.limitHigh = 0xFFFF;
    initDesc.limitLow = 0xF;
    initDesc.baseHigh = 0x0;
    initDesc.baseLow = 0x0;

    //64 bit code segment
    SegDescriptor csDesc = initDesc;
    csDesc.type.codeOrData = 1;
    csDesc.dpl = 0;
    //Because we're dealing with a pointer and I don't think it's
    //guaranteed that there isn't anything in a nonvirtual class between
    //it's beginning in memory and it's actual data, we'll use an
    //intermediary.
    uint64_t csDescVal = csDesc;
    physProxy.writeBlob(GDTBase + numGDTEntries * 8,
                        (uint8_t *)(&csDescVal), 8);

    numGDTEntries++;

    SegSelector cs = 0;
    cs.si = numGDTEntries - 1;

    tc->setMiscReg(MISCREG_CS, (MiscReg)cs);

    //32 bit data segment
    SegDescriptor dsDesc = initDesc;
    uint64_t dsDescVal = dsDesc;
    physProxy.writeBlob(GDTBase + numGDTEntries * 8,
                        (uint8_t *)(&dsDescVal), 8);

    numGDTEntries++;

    SegSelector ds = 0;
    ds.si = numGDTEntries - 1;

    tc->setMiscReg(MISCREG_DS, (MiscReg)ds);
    tc->setMiscReg(MISCREG_ES, (MiscReg)ds);
    tc->setMiscReg(MISCREG_FS, (MiscReg)ds);
    tc->setMiscReg(MISCREG_GS, (MiscReg)ds);
    tc->setMiscReg(MISCREG_SS, (MiscReg)ds);

    tc->setMiscReg(MISCREG_TSL, 0);
    tc->setMiscReg(MISCREG_TSG_BASE, GDTBase);
    tc->setMiscReg(MISCREG_TSG_LIMIT, 8 * numGDTEntries - 1);

    SegDescriptor tssDesc = initDesc;
    uint64_t tssDescVal = tssDesc;
    physProxy.writeBlob(GDTBase + numGDTEntries * 8,
                        (uint8_t *)(&tssDescVal), 8);

    numGDTEntries++;

    SegSelector tss = 0;
    tss.si = numGDTEntries - 1;

    tc->setMiscReg(MISCREG_TR, (MiscReg)tss);
    installSegDesc(tc, SYS_SEGMENT_REG_TR, tssDesc, true);

    /*
     * Identity map the first 4GB of memory. In order to map this region
     * of memory in long mode, there needs to be one actual page map level
     * 4 entry which points to one page directory pointer table which
     * points to 4 different page directory tables which are full of two
     * megabyte pages. All of the other entries in valid tables are set
     * to indicate that they don't pertain to anything valid and will
     * cause a fault if used.
     */

    // Put valid values in all of the various table entries which indicate
    // that those entries don't point to further tables or pages. Then
    // set the values of those entries which are needed.

    // Page Map Level 4

    // read/write, user, not present
    uint64_t pml4e = X86ISA::htog(0x6);
    for (int offset = 0; offset < (1 << PML4Bits) * 8; offset += 8) {
        physProxy.writeBlob(PageMapLevel4 + offset, (uint8_t *)(&pml4e), 8);
    }
    // Point to the only PDPT
    pml4e = X86ISA::htog(0x7 | PageDirPtrTable);
    physProxy.writeBlob(PageMapLevel4, (uint8_t *)(&pml4e), 8);

    // Page Directory Pointer Table

    // read/write, user, not present
    uint64_t pdpe = X86ISA::htog(0x6);
    for (int offset = 0; offset < (1 << PDPTBits) * 8; offset += 8) {
        physProxy.writeBlob(PageDirPtrTable + offset,
                            (uint8_t *)(&pdpe), 8);
    }
    // Point to the PDTs
    for (int table = 0; table < NumPDTs; table++) {
        pdpe = X86ISA::htog(0x7 | PageDirTable[table]);
        physProxy.writeBlob(PageDirPtrTable + table * 8,
                            (uint8_t *)(&pdpe), 8);
    }

    // Page Directory Tables

    Addr base = 0;
    const Addr pageSize = 2 << 20;
    for (int table = 0; table < NumPDTs; table++) {
        for (int offset = 0; offset < (1 << PDTBits) * 8; offset += 8) {
            // read/write, user, present, 4MB
            uint64_t pdte = X86ISA::htog(0x87 | base);
            physProxy.writeBlob(PageDirTable[table] + offset,
                                (uint8_t *)(&pdte), 8);
            base += pageSize;
        }
    }

    /*
     * Transition from real mode all the way up to Long mode
     */
    CR0 cr0 = tc->readMiscRegNoEffect(MISCREG_CR0);
    //Turn off paging.
    cr0.pg = 0;
    tc->setMiscReg(MISCREG_CR0, cr0);
    //Turn on protected mode.
    cr0.pe = 1;
    tc->setMiscReg(MISCREG_CR0, cr0);

    CR4 cr4 = tc->readMiscRegNoEffect(MISCREG_CR4);
    //Turn on pae.
    cr4.pae = 1;
    tc->setMiscReg(MISCREG_CR4, cr4);

    //Point to the page tables.
    tc->setMiscReg(MISCREG_CR3, PageMapLevel4);

    Efer efer = tc->readMiscRegNoEffect(MISCREG_EFER);
    //Enable long mode.
    efer.lme = 1;
    tc->setMiscReg(MISCREG_EFER, efer);

    //Start using longmode segments.
    installSegDesc(tc, SEGMENT_REG_CS, csDesc, true);
    installSegDesc(tc, SEGMENT_REG_DS, dsDesc, true);
    installSegDesc(tc, SEGMENT_REG_ES, dsDesc, true);
    installSegDesc(tc, SEGMENT_REG_FS, dsDesc, true);
    installSegDesc(tc, SEGMENT_REG_GS, dsDesc, true);
    installSegDesc(tc, SEGMENT_REG_SS, dsDesc, true);

    //Activate long mode.
    cr0.pg = 1;
    tc->setMiscReg(MISCREG_CR0, cr0);

    tc->pcState(tc->getSystemPtr()->kernelEntry);

    // We should now be in long mode. Yay!

    // Write out all valid config data (size!=0) provided by PRM
    // e.g. SMBios/DMI, IntelMP, E820
    for (int i=0; i<cp->getSegmentCount(DSid); i++) {
        const Segment *seg = cp->getSegment(DSid, i);
        if (seg->size)
            writeOutSegment(DSid, seg->base, seg->size, seg->offset);
    }

    // Set MTRR Register
    tc->setMiscReg(MISCREG_MTRR_PHYS_BASE_0, 0x0000000000000006);       // Base: 0x0, Type: WriteBack
    tc->setMiscReg(MISCREG_MTRR_PHYS_MASK_0, 0xFFFFFFFF80000800);       // Size: 2048MB, VALID
    tc->setMiscReg(MISCREG_MTRR_PHYS_BASE_1, 0x0000000080000006);       // Base: 2048MB, Type: WriteBack
    tc->setMiscReg(MISCREG_MTRR_PHYS_MASK_1, 0xFFFFFFFFC0000800);       // Size: 1024MB, VALID
    tc->setMiscReg(MISCREG_MTRR_PHYS_BASE_2, 0x00000000C0000000);       // Base: 3072MB, Type: Uncache
    tc->setMiscReg(MISCREG_MTRR_PHYS_MASK_2, 0xFFFFFFFFC0000800);       // Size: 1024MB, VALID

    // The location of the real mode data structure.
    const Addr realModeData = 0x90200;
    tc->setIntReg(INTREG_RSI, realModeData);
}

void
PARDg5VSystem::writeOutSegment(uint16_t DSid,
                               Addr base, int size, int offset)
{
    const uint8_t *ptr = NULL;

    // get config memory ptr
    ptr = cp->getConfigMem(offset, size);
    panic_if(!ptr, "Error read config memory @ 0x%x size 0x%x.\n",
                   offset, size);

    // write out to system memory
    physProxy.updateDSid(DSid);
    physProxy.writeBlob(base, ptr, size);

    DPRINTF(PARDg5VSystem, "writeOutSegment DSid=0x%x,"
                           "base=0x%x, size=0x%x, offset=0x%x\n",
                           DSid, base, size, offset);
}


PARDg5VSystem *
PARDg5VSystemParams::create()
{
    return new PARDg5VSystem(this);
}

