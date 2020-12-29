/*
 * Copyright (c) 2020-2021 saintube
 * Copyright (c) 2018 Inria
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
 */

/**
 * @file
 * Definitions of a scatter associative indexing policy.
 */

#include "mem/cache/tags/indexing_policies/scatter_associative.hh"

#include "base/bitfield.hh"
#include "base/intmath.hh"
#include "base/logging.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"

ScatterAssociative::ScatterAssociative(const Params *p)
    : BaseIndexingPolicy(p), msbShift(floorLog2(numSets) - 1)
{
    if (assoc > NUM_SCATTERING_FUNCTIONS) {
        warn_once("Associativity higher than number of scattering " \
                  "functions. Expect sub-optimal scattering.\n");
    }

    // Check if set is too big to do scattering. If using big sets, rewrite
    // scattering functions accordingly to make good use of the hashing
    // function
    panic_if(setShift + 2 * (msbShift + 1) > 64, "Unsuported number of bits " \
             "for the scattering functions.");

    // We must have more than two sets, otherwise the MSB and LSB are the same
    // bit, and the xor of them will always be 0
    fatal_if(numSets <= 2, "The number of sets must be greater than 2");
}

Addr
ScatterAssociative::hash(const Addr addr) const
{
    // Get relevant bits
    const uint8_t lsb = bits<Addr>(addr, 0);
    const uint8_t msb = bits<Addr>(addr, msbShift);
    const uint8_t xor_bit = msb ^ lsb;

    // Shift-off LSB and set new MSB as xor of old LSB and MSB
    return insertBits<Addr, uint8_t>(addr >> 1, msbShift, xor_bit);
}

Addr
ScatterAssociative::dehash(const Addr addr) const
{
    // Get relevant bits. The original MSB is one bit away on the current MSB
    // (which is the XOR bit). The original LSB can be retrieved from inverting
    // the xor that generated the XOR bit.
    const uint8_t msb = bits<Addr>(addr, msbShift - 1);
    const uint8_t xor_bit = bits<Addr>(addr, msbShift);
    const uint8_t lsb = msb ^ xor_bit;

    // Remove current MSB (XOR bit), shift left and add LSB back
    const Addr addr_no_msb = mbits<Addr>(addr, msbShift - 1, 0);
    return insertBits<Addr, uint8_t>(addr_no_msb << 1, 0, lsb);
}

Addr
ScatterAssociative::scatter(const Addr addr, const uint32_t way) const
{
    // Assume an address of size A bits can be decomposed into
    // {addr3, addr2, addr1, addr0}, where:
    //   addr0 (M bits) = Block offset;
    //   addr1 (N bits) = Set bits in conventional cache;
    //   addr3 (A - M - 2*N bits), addr2 (N bits) = Tag bits.
    // We use addr1 and addr2, as proposed in the original paper
    Addr addr1 = bits<Addr>(addr, msbShift, 0);
    const Addr addr2 = bits<Addr>(addr, 2 * (msbShift + 1) - 1, msbShift + 1);

    // Select and apply scattering function for given way
    switch (way % NUM_SCATTERING_FUNCTIONS) {
      case 0:
        addr1 = hash(addr1) ^ hash(addr2) ^ addr2;
        break;
      case 1:
        addr1 = hash(addr1) ^ hash(addr2) ^ addr1;
        break;
      case 2:
        addr1 = hash(addr1) ^ dehash(addr2) ^ addr2;
        break;
      case 3:
        addr1 = hash(addr1) ^ dehash(addr2) ^ addr1;
        break;
      case 4:
        addr1 = dehash(addr1) ^ hash(addr2) ^ addr2;
        break;
      case 5:
        addr1 = dehash(addr1) ^ hash(addr2) ^ addr1;
        break;
      case 6:
        addr1 = dehash(addr1) ^ dehash(addr2) ^ addr2;
        break;
      case 7:
        addr1 = dehash(addr1) ^ dehash(addr2) ^ addr1;
        break;
      default:
        panic("A scattering function has not been implemented for this way.");
    }

    // If we have more than 8 ways, just pile them up on hashes. This is not
    // the optimal solution, and can be improved by adding more scattering
    // functions to the previous selector
    for (uint32_t i = 0; i < way/NUM_SCATTERING_FUNCTIONS; i++) {
        addr1 = hash(addr1);
    }

    return addr1;
}

Addr
ScatterAssociative::descatter(const Addr addr, const uint32_t way) const
{
    // Get relevant bits of the addr
    Addr addr1 = bits<Addr>(addr, msbShift, 0);
    const Addr addr2 = bits<Addr>(addr, 2 * (msbShift + 1) - 1, msbShift + 1);

    // If we have more than NUM_SCATTERING_FUNCTIONS ways, unpile the hashes
    if (way >= NUM_SCATTERING_FUNCTIONS) {
        for (uint32_t i = 0; i < way/NUM_SCATTERING_FUNCTIONS; i++) {
            addr1 = dehash(addr1);
        }
    }

    // Select and apply scattering function for given way
    switch (way % 8) {
      case 0:
        return dehash(addr1 ^ hash(addr2) ^ addr2);
      case 1:
        addr1 = addr1 ^ hash(addr2);
        for (int i = 0; i < msbShift; i++) {
            addr1 = hash(addr1);
        }
        return addr1;
      case 2:
        return dehash(addr1 ^ dehash(addr2) ^ addr2);
      case 3:
        addr1 = addr1 ^ dehash(addr2);
        for (int i = 0; i < msbShift; i++) {
            addr1 = hash(addr1);
        }
        return addr1;
      case 4:
        return hash(addr1 ^ hash(addr2) ^ addr2);
      case 5:
        addr1 = addr1 ^ hash(addr2);
        for (int i = 0; i <= msbShift; i++) {
            addr1 = hash(addr1);
        }
        return addr1;
      case 6:
        return hash(addr1 ^ dehash(addr2) ^ addr2);
      case 7:
        addr1 = addr1 ^ dehash(addr2);
        for (int i = 0; i <= msbShift; i++) {
            addr1 = hash(addr1);
        }
        return addr1;
      default:
        panic("A scattering function has not been implemented for this way.");
    }
}

uint32_t
ScatterAssociative::extractSet(const Addr addr, const uint32_t way) const
{
    return scatter(addr >> setShift, way) & setMask;
}

Addr
ScatterAssociative::regenerateAddr(const Addr tag,
                                  const ReplaceableEntry* entry) const
{
    const Addr addr_set = (tag << (msbShift + 1)) | entry->getSet();
    return (tag << tagShift) |
           ((descatter(addr_set, entry->getWay()) & setMask) << setShift);
}

std::vector<ReplaceableEntry*>
ScatterAssociative::getPossibleEntries(const Addr addr) const
{
    std::vector<ReplaceableEntry*> entries;

    // Parse all ways
    for (uint32_t way = 0; way < assoc; ++way) {
        // Apply hash to get set, and get way entry in it
        entries.push_back(sets[extractSet(addr, way)][way]);
    }

    return entries;
}

ScatterAssociative *
ScatterAssociativeParams::create()
{
    return new ScatterAssociative(this);
}
