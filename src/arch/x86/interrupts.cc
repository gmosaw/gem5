/*
 * Copyright (c) 2008 The Hewlett-Packard Development Company
 * All rights reserved.
 *
 * Redistribution and use of this software in source and binary forms,
 * with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * The software must be used only for Non-Commercial Use which means any
 * use which is NOT directed to receiving any direct monetary
 * compensation for, or commercial advantage from such use.  Illustrative
 * examples of non-commercial use are academic research, personal study,
 * teaching, education and corporate research & development.
 * Illustrative examples of commercial use are distributing products for
 * commercial advantage and providing services using the software for
 * commercial advantage.
 *
 * If you wish to use this software or functionality therein that may be
 * covered by patents for commercial use, please contact:
 *     Director of Intellectual Property Licensing
 *     Office of Strategy and Technology
 *     Hewlett-Packard Company
 *     1501 Page Mill Road
 *     Palo Alto, California  94304
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.  Redistributions
 * in binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.  Neither the name of
 * the COPYRIGHT HOLDER(s), HEWLETT-PACKARD COMPANY, nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.  No right of
 * sublicense is granted herewith.  Derivatives of the software and
 * output created using the software may be prepared, but only for
 * Non-Commercial Uses.  Derivatives of the software may be shared with
 * others provided: (i) the others agree to abide by the list of
 * conditions herein which includes the Non-Commercial Use restrictions;
 * and (ii) such Derivatives of the software include the above copyright
 * notice to acknowledge the contribution from this software where
 * applicable, this list of conditions and the disclaimer below.
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
 * Authors: Gabe Black
 */

#include "arch/x86/apicregs.hh"
#include "arch/x86/interrupts.hh"
#include "arch/x86/intmessage.hh"
#include "cpu/base.hh"
#include "mem/packet_access.hh"

int
divideFromConf(uint32_t conf)
{
    // This figures out what division we want from the division configuration
    // register in the local APIC. The encoding is a little odd but it can
    // be deciphered fairly easily.
    int shift = ((conf & 0x8) >> 1) | (conf & 0x3);
    shift = (shift + 1) % 8;
    return 1 << shift;
}

namespace X86ISA
{

ApicRegIndex
decodeAddr(Addr paddr)
{
    ApicRegIndex regNum;
    paddr &= ~mask(3);
    switch (paddr)
    {
      case 0x20:
        regNum = APIC_ID;
        break;
      case 0x30:
        regNum = APIC_VERSION;
        break;
      case 0x80:
        regNum = APIC_TASK_PRIORITY;
        break;
      case 0x90:
        regNum = APIC_ARBITRATION_PRIORITY;
        break;
      case 0xA0:
        regNum = APIC_PROCESSOR_PRIORITY;
        break;
      case 0xB0:
        regNum = APIC_EOI;
        break;
      case 0xD0:
        regNum = APIC_LOGICAL_DESTINATION;
        break;
      case 0xE0:
        regNum = APIC_DESTINATION_FORMAT;
        break;
      case 0xF0:
        regNum = APIC_SPURIOUS_INTERRUPT_VECTOR;
        break;
      case 0x100:
      case 0x108:
      case 0x110:
      case 0x118:
      case 0x120:
      case 0x128:
      case 0x130:
      case 0x138:
      case 0x140:
      case 0x148:
      case 0x150:
      case 0x158:
      case 0x160:
      case 0x168:
      case 0x170:
      case 0x178:
        regNum = APIC_IN_SERVICE((paddr - 0x100) / 0x8);
        break;
      case 0x180:
      case 0x188:
      case 0x190:
      case 0x198:
      case 0x1A0:
      case 0x1A8:
      case 0x1B0:
      case 0x1B8:
      case 0x1C0:
      case 0x1C8:
      case 0x1D0:
      case 0x1D8:
      case 0x1E0:
      case 0x1E8:
      case 0x1F0:
      case 0x1F8:
        regNum = APIC_TRIGGER_MODE((paddr - 0x180) / 0x8);
        break;
      case 0x200:
      case 0x208:
      case 0x210:
      case 0x218:
      case 0x220:
      case 0x228:
      case 0x230:
      case 0x238:
      case 0x240:
      case 0x248:
      case 0x250:
      case 0x258:
      case 0x260:
      case 0x268:
      case 0x270:
      case 0x278:
        regNum = APIC_INTERRUPT_REQUEST((paddr - 0x200) / 0x8);
        break;
      case 0x280:
        regNum = APIC_ERROR_STATUS;
        break;
      case 0x300:
        regNum = APIC_INTERRUPT_COMMAND_LOW;
        break;
      case 0x310:
        regNum = APIC_INTERRUPT_COMMAND_HIGH;
        break;
      case 0x320:
        regNum = APIC_LVT_TIMER;
        break;
      case 0x330:
        regNum = APIC_LVT_THERMAL_SENSOR;
        break;
      case 0x340:
        regNum = APIC_LVT_PERFORMANCE_MONITORING_COUNTERS;
        break;
      case 0x350:
        regNum = APIC_LVT_LINT0;
        break;
      case 0x360:
        regNum = APIC_LVT_LINT1;
        break;
      case 0x370:
        regNum = APIC_LVT_ERROR;
        break;
      case 0x380:
        regNum = APIC_INITIAL_COUNT;
        break;
      case 0x390:
        regNum = APIC_CURRENT_COUNT;
        break;
      case 0x3E0:
        regNum = APIC_DIVIDE_CONFIGURATION;
        break;
      default:
        // A reserved register field.
        panic("Accessed reserved register field %#x.\n", paddr);
        break;
    }
    return regNum;
}
}

Tick
X86ISA::Interrupts::read(PacketPtr pkt)
{
    Addr offset = pkt->getAddr() - pioAddr;
    //Make sure we're at least only accessing one register.
    if ((offset & ~mask(3)) != ((offset + pkt->getSize()) & ~mask(3)))
        panic("Accessed more than one register at a time in the APIC!\n");
    ApicRegIndex reg = decodeAddr(offset);
    uint32_t val = htog(readReg(reg));
    DPRINTF(LocalApic,
            "Reading Local APIC register %d at offset %#x as %#x.\n",
            reg, offset, val);
    pkt->setData(((uint8_t *)&val) + (offset & mask(3)));
    return latency;
}

Tick
X86ISA::Interrupts::write(PacketPtr pkt)
{
    Addr offset = pkt->getAddr() - pioAddr;
    //Make sure we're at least only accessing one register.
    if ((offset & ~mask(3)) != ((offset + pkt->getSize()) & ~mask(3)))
        panic("Accessed more than one register at a time in the APIC!\n");
    ApicRegIndex reg = decodeAddr(offset);
    uint32_t val = regs[reg];
    pkt->writeData(((uint8_t *)&val) + (offset & mask(3)));
    DPRINTF(LocalApic,
            "Writing Local APIC register %d at offset %#x as %#x.\n",
            reg, offset, gtoh(val));
    setReg(reg, gtoh(val));
    return latency;
}

Tick
X86ISA::Interrupts::recvMessage(PacketPtr pkt)
{
    uint8_t id = 0;
    Addr offset = pkt->getAddr() - x86InterruptAddress(id, 0);
    assert(pkt->cmd == MemCmd::MessageReq);
    switch(offset)
    {
      case 0:
        {
            TriggerIntMessage message = pkt->get<TriggerIntMessage>();
            uint8_t vector = message.vector;
            DPRINTF(LocalApic,
                    "Got Trigger Interrupt message with vector %#x.\n",
                    vector);
            // Make sure we're really supposed to get this.
            assert((message.destMode == 0 && message.destination == id) ||
                   (bits((int)message.destination, id)));
            if (DeliveryMode::isUnmaskable(message.deliveryMode)) {
                DPRINTF(LocalApic, "Interrupt is an %s and unmaskable.\n",
                        DeliveryMode::names[message.deliveryMode]);
                panic("Unmaskable interrupts aren't implemented.\n");
            } else if (DeliveryMode::isMaskable(message.deliveryMode)) {
                DPRINTF(LocalApic, "Interrupt is an %s and maskable.\n",
                        DeliveryMode::names[message.deliveryMode]);
                // Queue up the interrupt in the IRR.
                if (vector > IRRV)
                    IRRV = vector;
                if (!getRegArrayBit(APIC_INTERRUPT_REQUEST_BASE, vector)) {
                    setRegArrayBit(APIC_INTERRUPT_REQUEST_BASE, vector);
                    if (message.trigger) {
                        // Level triggered.
                        setRegArrayBit(APIC_TRIGGER_MODE_BASE, vector);
                    } else {
                        // Edge triggered.
                        clearRegArrayBit(APIC_TRIGGER_MODE_BASE, vector);
                    }
                }
            }
        }
        break;
      default:
        panic("Local apic got unknown interrupt message at offset %#x.\n",
                offset);
        break;
    }
    delete pkt->req;
    delete pkt;
    return latency;
}


uint32_t
X86ISA::Interrupts::readReg(ApicRegIndex reg)
{
    if (reg >= APIC_TRIGGER_MODE(0) &&
            reg <= APIC_TRIGGER_MODE(15)) {
        panic("Local APIC Trigger Mode registers are unimplemented.\n");
    }
    switch (reg) {
      case APIC_ARBITRATION_PRIORITY:
        panic("Local APIC Arbitration Priority register unimplemented.\n");
        break;
      case APIC_PROCESSOR_PRIORITY:
        panic("Local APIC Processor Priority register unimplemented.\n");
        break;
      case APIC_EOI:
        panic("Local APIC EOI register unimplemented.\n");
        break;
      case APIC_ERROR_STATUS:
        regs[APIC_INTERNAL_STATE] &= ~ULL(0x1);
        break;
      case APIC_INTERRUPT_COMMAND_LOW:
        panic("Local APIC Interrupt Command low"
                " register unimplemented.\n");
        break;
      case APIC_INTERRUPT_COMMAND_HIGH:
        panic("Local APIC Interrupt Command high"
                " register unimplemented.\n");
        break;
      case APIC_CURRENT_COUNT:
        {
            assert(clock);
            uint32_t val = regs[reg] - curTick / clock;
            val /= (16 * divideFromConf(regs[APIC_DIVIDE_CONFIGURATION]));
            return val;
        }
      default:
        break;
    }
    return regs[reg];
}

void
X86ISA::Interrupts::setReg(ApicRegIndex reg, uint32_t val)
{
    uint32_t newVal = val;
    if (reg >= APIC_IN_SERVICE(0) &&
            reg <= APIC_IN_SERVICE(15)) {
        panic("Local APIC In-Service registers are unimplemented.\n");
    }
    if (reg >= APIC_TRIGGER_MODE(0) &&
            reg <= APIC_TRIGGER_MODE(15)) {
        panic("Local APIC Trigger Mode registers are unimplemented.\n");
    }
    if (reg >= APIC_INTERRUPT_REQUEST(0) &&
            reg <= APIC_INTERRUPT_REQUEST(15)) {
        panic("Local APIC Interrupt Request registers "
                "are unimplemented.\n");
    }
    switch (reg) {
      case APIC_ID:
        newVal = val & 0xFF;
        break;
      case APIC_VERSION:
        // The Local APIC Version register is read only.
        return;
      case APIC_TASK_PRIORITY:
        newVal = val & 0xFF;
        break;
      case APIC_ARBITRATION_PRIORITY:
        panic("Local APIC Arbitration Priority register unimplemented.\n");
        break;
      case APIC_PROCESSOR_PRIORITY:
        panic("Local APIC Processor Priority register unimplemented.\n");
        break;
      case APIC_EOI:
        panic("Local APIC EOI register unimplemented.\n");
        break;
      case APIC_LOGICAL_DESTINATION:
        newVal = val & 0xFF000000;
        break;
      case APIC_DESTINATION_FORMAT:
        newVal = val | 0x0FFFFFFF;
        break;
      case APIC_SPURIOUS_INTERRUPT_VECTOR:
        regs[APIC_INTERNAL_STATE] &= ~ULL(1 << 1);
        regs[APIC_INTERNAL_STATE] |= val & (1 << 8);
        if (val & (1 << 9))
            warn("Focus processor checking not implemented.\n");
        break;
      case APIC_ERROR_STATUS:
        {
            if (regs[APIC_INTERNAL_STATE] & 0x1) {
                regs[APIC_INTERNAL_STATE] &= ~ULL(0x1);
                newVal = 0;
            } else {
                regs[APIC_INTERNAL_STATE] |= ULL(0x1);
                return;
            }

        }
        break;
      case APIC_INTERRUPT_COMMAND_LOW:
        panic("Local APIC Interrupt Command low"
                " register unimplemented.\n");
        break;
      case APIC_INTERRUPT_COMMAND_HIGH:
        panic("Local APIC Interrupt Command high"
                " register unimplemented.\n");
        break;
      case APIC_LVT_TIMER:
      case APIC_LVT_THERMAL_SENSOR:
      case APIC_LVT_PERFORMANCE_MONITORING_COUNTERS:
      case APIC_LVT_LINT0:
      case APIC_LVT_LINT1:
      case APIC_LVT_ERROR:
        {
            uint64_t readOnlyMask = (1 << 12) | (1 << 14);
            newVal = (val & ~readOnlyMask) |
                     (regs[reg] & readOnlyMask);
        }
        break;
      case APIC_INITIAL_COUNT:
        {
            assert(clock);
            newVal = bits(val, 31, 0);
            uint32_t newCount = newVal *
                (divideFromConf(regs[APIC_DIVIDE_CONFIGURATION]) * 16);
            regs[APIC_CURRENT_COUNT] = newCount + curTick / clock;
            // Find out how long a "tick" of the timer should take.
            Tick timerTick = 16 * clock;
            // Schedule on the edge of the next tick plus the new count.
            Tick offset = curTick % timerTick;
            if (offset) {
                reschedule(apicTimerEvent,
                        curTick + (newCount + 1) * timerTick - offset, true);
            } else {
                reschedule(apicTimerEvent,
                        curTick + newCount * timerTick, true);
            }
        }
        break;
      case APIC_CURRENT_COUNT:
        //Local APIC Current Count register is read only.
        return;
      case APIC_DIVIDE_CONFIGURATION:
        newVal = val & 0xB;
        break;
      default:
        break;
    }
    regs[reg] = newVal;
    return;
}

bool
X86ISA::Interrupts::check_interrupts(ThreadContext * tc) const
{
    RFLAGS rflags = tc->readMiscRegNoEffect(MISCREG_RFLAGS);
    if (IRRV > ISRV && rflags.intf &&
            bits(IRRV, 7, 4) > bits(regs[APIC_TASK_PRIORITY], 7, 4)) {
        return true;
    }
    return false;
}

Fault
X86ISA::Interrupts::getInterrupt(ThreadContext * tc)
{
    assert(check_interrupts(tc));
    return new ExternalInterrupt(IRRV);
}

void
X86ISA::Interrupts::updateIntrInfo(ThreadContext * tc)
{
    assert(check_interrupts(tc));
    // Mark the interrupt as "in service".
    ISRV = IRRV;
    setRegArrayBit(APIC_IN_SERVICE_BASE, ISRV);
    // Clear it out of the IRR.
    clearRegArrayBit(APIC_INTERRUPT_REQUEST_BASE, IRRV);
    updateIRRV();
}

X86ISA::Interrupts *
X86LocalApicParams::create()
{
    return new X86ISA::Interrupts(this);
}
