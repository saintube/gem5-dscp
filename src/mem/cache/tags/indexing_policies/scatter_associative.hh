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
 * Declaration of a scatter associative indexing policy.
 */

#ifndef __MEM_CACHE_INDEXING_POLICIES_SCATTER_ASSOCIATIVE_HH__
#define __MEM_CACHE_INDEXING_POLICIES_SCATTER_ASSOCIATIVE_HH__

#include <vector>

#include "mem/cache/tags/indexing_policies/base.hh"
#include "params/ScatterAssociative.hh"

class ReplaceableEntry;

/**
 * A scatter associative indexing policy.
 * @sa  \ref gem5MemorySystem "gem5 Memory System"
 *
 * The scatter indexing policy has a variable mapping based on a hash function,
 * so a value x can be mapped to different sets, based on the way being used.
 *
 * For example, let's assume address A maps to set 3 on way 0. It will likely
 * have a different set for every other way. Visually, the possible locations
 * of A are, for a table with 4 ways and 8 sets (arbitrarily chosen sets; these
 * locations depend on A and the hashing function used):
 *    Way 0   1   2   3
 *  Set   _   _   _   _
 *    0  |_| |_| |X| |_|
 *    1  |_| |_| |_| |X|
 *    2  |_| |_| |_| |_|
 *    3  |X| |_| |_| |_|
 *    4  |_| |_| |_| |_|
 *    5  |_| |X| |_| |_|
 *    6  |_| |_| |_| |_|
 *    7  |_| |_| |_| |_|
 *
 * If provided with an associativity higher than the number of scattering
 * functions, the scattering functions of the extra ways might be sub-optimal.
 */
class ScatterAssociative : public BaseIndexingPolicy
{
  private:
    /**
     * The number of scattering functions implemented. Should be updated if
     * more functions are added. If more than this number of scattering
     * functions are needed (i.e., assoc > this value), we programatically
     * generate new ones, which may be sub-optimal.
     */
    const int NUM_SCATTERING_FUNCTIONS = 8;

    /**
     * The amount to shift a set index to get its MSB.
     */
    const int msbShift;

    /**
     * The hash function itself. Uses the hash function H, as described in
     * "Scatter-Associative Caches", from Seznec et al. (section 3.3): It
     * applies an XOR to the MSB and LSB, shifts all bits one bit to the right,
     * and set the result of the XOR as the new MSB.
     *
     * This function is not bijective if the address has only 1 bit, as the MSB
     * and LSB will be the same, and therefore the xor will always be 0.
     *
     * @param addr The address to be hashed.
     * @param The hashed address.
     */
    Addr hash(const Addr addr) const;

    /**
     * Inverse of the hash function.
     * @sa hash().
     *
     * @param addr The address to be dehashed.
     * @param The dehashed address.
     */
    Addr dehash(const Addr addr) const;

    /**
     * Address scattering function selection. It selects and applies one of the
     * scattering functions based on the way provided.
     *
     * @param addr Address to be scattered. Should contain the set and tag
     * bits.
     * @param way The cache way, used to select a hash function.
     * @return The scattered address.
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
     * Apply a scattering function to calculate address' set given a way.
     *
     * @param addr The address to calculate the set for.
     * @param way The way to get the set from.
     * @return The set index for given combination of address and way.
     */
    uint32_t extractSet(const Addr addr, const uint32_t way) const;

  public:
    /** Convenience typedef. */
     typedef ScatterAssociativeParams Params;

    /**
     * Construct and initialize this policy.
     */
    ScatterAssociative(const Params *p);

    /**
     * Destructor.
     */
    ~ScatterAssociative() {};

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
     * Uses the inverse of the scattering function.
     *
     * @param tag The tag bits.
     * @param entry The entry.
     * @return the entry's address.
     */
    Addr regenerateAddr(const Addr tag, const ReplaceableEntry* entry) const
                                                                   override;
};

#endif //__MEM_CACHE_INDEXING_POLICIES_SCATTER_ASSOCIATIVE_HH__
