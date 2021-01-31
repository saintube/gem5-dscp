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
 * Declaration of a DSCP indexing policy.
 */

#ifndef __MEM_CACHE_INDEXING_POLICIES_DSCP_HH__
#define __MEM_CACHE_INDEXING_POLICIES_DSCP_HH__

#include <vector>

#include "mem/cache/cache_blk.hh"
#include "mem/cache/tags/indexing_policies/base.hh"
#include "mem/cache/tags/indexing_policies/qarma64.hh"
#include "params/DSCP.hh"

class ReplaceableEntry;

/**
 * A partitioned scatter associative indexing policy.
 * @sa  \ref gem5MemorySystem "gem5 Memory System"
 *
 * The DSCP indexing policy has a variable mapping based on a block cipher
 * and a partitioning cache, so a value x can be mapped to different sets,
 * based on the way being used.
 *
 */
class DSCP : public BaseIndexingPolicy
{
  private:
    /**
     * The instance of tweakable block cipher for scattering computation.
     */
    Qarma64* cipher;

    /**
     * Default key of the cipher instance.
     * TODO: make it configurable.
     */
    const Addr W0 = 0x84be85ce9804e94b;
    const Addr K0 = 0xec2802d4e0a488e9;

    /**
     * The number of encryption rounds.
     */
    const int NUM_ENC_ROUNDS = 5;

    /**
     * The amount to shift a set index to get its MSB.
     */
    const int msbShift;

    /**
     * The scatter function intends to compute the set indices as a
     * permutation of the address with a block cipher encryption algorithm.
     *
     * @param addr Address to be scattered. Should contain the set and tag
     * bits.
     * @param way The cache way, used to consititute the cipher's tweak.
     * @return The scattered set address.
     */
    Addr scatter(const Addr addr, const uint32_t way) const;

    /**
     * Address descattering function (inverse of the scatter function) of the
     * given way.
     * @sa scatter()
     *
     * @param addr Address to be descattered. Should contain the set and tag
     * bits.
     * @param way The cache way, used to select a hash function.
     * @return The descattered address.
     */
    Addr descatter(const Addr addr, const uint32_t way) const;

    /**
     * Use scattering and PLC to calculate address' set given a way.
     *
     * @param addr The address to calculate the set for.
     * @param way The way to get the set from.
     * @return The set index for given combination of address and way.
     */
    uint32_t extractSet(const Addr addr, const uint32_t way) const;

  public:
    /** Convenience typedef. */
     typedef DSCPParams Params;

    /**
     * Construct and initialize this policy.
     */
    DSCP(const Params *p);

    /**
     * Destructor.
     */
    ~DSCP() {};

    /**
     * Generate the tag from the given address.
     *
     * @param addr The address to get the tag from.
     * @return The tag of the address.
     */
    Addr extractTag(const Addr addr) const override;

    /**
     * Get the sets belonging to the sector.
     *
     * @param secId The sector ID.
     * @return the sets' blks belonging to the sector.
     */
    std::vector<CacheBlk*> getSectorSets(int secId) const override;

    /**
     * Find all possible entries for insertion and replacement of an address.
     * Should be called immediately before ReplacementPolicy's findVictim()
     * not to break cache resizing.
     *
     * @param addr The addr to a find possible entries for.
     * @return The possible entries.
     */
    std::vector<ReplaceableEntry*> getPossibleEntries(const Addr addr) const
                                                                   override;

    /**
     * Regenerate an entry's address from its tag and assigned set and way.
     * Uses the inverse of the scattering function or just the total tag.
     *
     * @param tag The tag bits.
     * @param entry The entry.
     * @return the entry's address.
     */
    Addr regenerateAddr(const Addr tag, const ReplaceableEntry* entry) const
                                                                   override;
};

#endif //__MEM_CACHE_INDEXING_POLICIES_DSCP_HH__
