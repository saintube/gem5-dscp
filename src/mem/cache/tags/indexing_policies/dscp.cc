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
 * Definitions of a DSCP indexing policy.
 */

#include "mem/cache/tags/indexing_policies/dscp.hh"

#include "base/bitfield.hh"
#include "base/intmath.hh"
#include "base/logging.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"

DSCP::DSCP(const Params *p)
    : BaseIndexingPolicy(p), msbShift(floorLog2(numSets) - 1)
{
    cipher = new Qarma64(W0, K0);

    // Check if set is too big to do scattering. If using big sets, rewrite
    // scattering functions accordingly to make good use of the hashing
    // function
    panic_if(setShift + 2 * (msbShift + 1) > 64, "Unsuported number of bits " \
             "for the scattering functions.");

    // We must have more than two sets, otherwise the MSB and LSB are the same
    // bit, and the xor of them will always be 0
    fatal_if(numSets <= 2, "The number of sets must be greater than 2");

    fatal_if(plc_size <= 0, "The size of PLC must be larger than 0");
    isPLCEnabled = true;
}

Addr
DSCP::scatter(const Addr addr, const uint32_t way) const
{
    /**
     * TODO: ScatterCache: concatenate SDID (maybe also SecID) with way index
     * as the tweak.
     */

    /**
     * SCv1: simply use both tag and index bits as the plaintext, whose output
     * has potential birthday-bound complexity.
     */
    // return cipher->qarma64_enc(addr, way, NUM_ENC_ROUNDS);

    /**
     * SCv2: only use index bits as the plaintext, while the tag bits
     * constitute the tweak in order to mitigate birthday-bound index
     * collisions.
     */
    Addr indexBits = (addr & setMask);
    Addr tweak = ((addr & (~setMask)) | way);
    return cipher->qarma64_enc(indexBits, tweak, NUM_ENC_ROUNDS);
}

Addr
DSCP::descatter(const Addr addr, const uint32_t way) const
{
    /**
     * NOTE: this function is not effectual presently since the complete addr
     * is recorded in the block.
     * TODO: reduce tag bits cost by a decryption algorithm.
     */
    return cipher->qarma64_dec(addr, way, NUM_ENC_ROUNDS);
}

uint32_t
DSCP::extractSet(const Addr addr, const uint32_t way) const
{
    /**
     * Scatter the address into partitioned sector.
     * NOTE: there should not have no plc miss
     */
    int secId = plc->getSector(addr);
    return secId * sectSets + (scatter(addr >> setShift, way) % sectSets);
}

Addr
DSCP::extractTag(const Addr addr) const
{
    /**
     * TODO: reduce tag bits cost.
     */
    return addr >> setShift;
}

Addr
DSCP::regenerateAddr(const Addr tag,
                                  const ReplaceableEntry* entry) const
{
    /*
    const Addr addr_set = (tag << (msbShift + 1)) | entry->getSet();
    return (tag << tagShift) |
           ((descatter(addr_set, entry->getWay()) & setMask) << setShift);
    */
    return tag << setShift;
}

std::vector<ReplaceableEntry*>
DSCP::getPossibleEntries(const Addr addr) const
{
    std::vector<ReplaceableEntry*> entries;

    // PLC misses
    int secId = plc->getSector(addr);
    if (secId < 0)
        return entries;

    // Parse all ways
    for (uint32_t way = 0; way < assoc; ++way) {
        // Apply hash to get set, and get way entry in it
        entries.push_back(sets[extractSet(addr, way)][way]);
    }

    return entries;
}

std::vector<CacheBlk*>
DSCP::getSectorSets(int secId) const
{
    std::vector<CacheBlk*> entries;
    if (secId < 0)
        return entries;

    // append all lines belonging to the sector sets
    for (unsigned i = secId * sectSets; i < (secId + 1) * sectSets; i++) {
        for (unsigned j = 0; j < assoc; j++) {
            // NOTE: ensure the blk type does not change
            CacheBlk* blk = static_cast<CacheBlk*>(sets[i][j]);
            if (blk->isValid())
                entries.push_back(blk);
        }
    }
    return entries;
}

bool
DSCP::accessSector(int secId)
{
    /**
     * TODO: suppose the sector exists and so do the promotion.
     */
    // the PLC is enabled
    return true;
}

int
DSCP::getVictimSector(Stats::Vector& contributions) const
{
    // The replacement is expensive since it takes O(n) get the victim
    // sector and erase entries.
    double minContr = 2147483647;
    int victim = -1;
    for (int i = 0; i < contributions.size(); i++) {
        if (!plc->getSC(i)) {
            // TODO: find a mostly-unused sector
            return i;
        }
        double contr = contributions[i].value();
        if (contr < minContr) {
            minContr = contr;
            victim = i;
        }
    }
    return victim;
}

DSCP *
DSCPParams::create()
{
    return new DSCP(this);
}
