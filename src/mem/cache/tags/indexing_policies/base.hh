/*
 * Copyright (c) 2018 Inria
 * Copyright (c) 2012-2014,2017 ARM Limited
 * All rights reserved.
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
 * Copyright (c) 2003-2005,2014 The Regents of The University of Michigan
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
 * Declaration of a common framework for indexing policies.
 */

#ifndef __MEM_CACHE_INDEXING_POLICIES_BASE_HH__
#define __MEM_CACHE_INDEXING_POLICIES_BASE_HH__

#include <map>
#include <vector>

#include "base/statistics.hh"
#include "mem/cache/cache_blk.hh"
#include "params/BaseIndexingPolicy.hh"
#include "sim/sim_object.hh"

class ReplaceableEntry;
class PLC;

/**
 * A auxiliary class for partitioned indexing table locations. Notice
 * that PLC is full-associative and has its replacement policy.
 */
class PLC
{
  private:
    /**
     * The capacity for PLC lines.
     */
    unsigned capacity;

    /**
     * The tag shift in PLC.
     */
    unsigned tagShift;

    /**
     * The tag mask in PLC.
     * NOTE: the length of unmasked bits affects the hit rate.
     */
    unsigned tagMask;

    /**
     * The sector shift in PLC.
     */
    unsigned pSectors;

    /**
     * Maitaining the hashing between partition owners and sector id.
     */
    std::map<unsigned, int> m;

    /**
     * The replacement counter.
     */
    int count;

    /**
     * The second chance bits.
     */
    std::vector<bool> sc;

  public:
    /**
     * Construct and initialize this policy.
     */
    PLC(unsigned psize, unsigned shift);

    /**
     * Destructor.
     */
    ~PLC() {};

    /**
     * isFull returns whether PLC is full or not.
     */
    bool isFull();

    /**
     * initSectors do the actual initialization like the configuration
     * for sectors.
     *
     * @param pSects the number of pSectors.
     */
    void initSectors(unsigned pSects);

    /**
     * getSector returns the sector id of PLC. It returns -1 if no
     * mapping for the given addr.
     * NOTE: the range of output is [-1, pSectors - 1].
     *
     * @param addr The entry's address.
     * @return the sector id.
     */
    int getSector(const Addr addr);

    /**
     * setPLCEntry sets the address field of given addr mapping to
     * given sector id.
     *
     * @param addr The entry's address.
     * @param secId the sector id.
     * @return whether the mapping changed or not.
     */
    bool setPLCEntry(const Addr addr, int secId);

    /**
     * deletePLCEntry deletes the PLC entry for the given addr.
     *
     * @param addr The entry's address.
     * @return whether the deletion is successful or not.
     */
    bool deletePLCEntry(const Addr addr);

    /**
     * deletePLCEntry deletes the PLC entry for the given sector.
     *
     * @param addr The entry's sector.
     * @return whether the deletion is successful or not.
     */
    bool deletePLCEntry(int secId);

    /**
     * setSC sets the second chance bit for the given sector.
     *
     * @param secId The sector id.
     * @param value the setting value.
     */
    void setSC(int secId, bool value);

    /**
     * getSC returns the second chance bit for the given sector.
     *
     * @param secId The sector id.
     * @return the sc bit.
     */
    bool getSC(int secId);
};

/**
 * A common base class for indexing table locations. Classes that inherit
 * from it determine hash functions that should be applied based on the set
 * and way. These functions are then applied to re-map the original values.
 * @sa  \ref gem5MemorySystem "gem5 Memory System"
 */
class BaseIndexingPolicy : public SimObject
{
  protected:
    /**
     * The associativity.
     */
    const unsigned assoc;

    /**
     * The number of sets in the cache.
     */
    const uint32_t numSets;

    /**
     * The amount to shift the address to get the set.
     */
    const int setShift;

    /**
     * Mask out all bits that aren't part of the set index.
     */
    const unsigned setMask;

    /**
     * The cache sets.
     */
    std::vector<std::vector<ReplaceableEntry*>> sets;

    /**
     * The amount to shift the address to get the tag.
     */
    const int tagShift;

    /**
     * The number of partition lookup cache lines.
     */
    const int plc_size;

  public:

    /**
     * Convenience typedef.
     */
    typedef BaseIndexingPolicyParams Params;

    /**
     * Construct and initialize this policy.
     */
    BaseIndexingPolicy(const Params *p);

    /**
     * Destructor.
     */
    ~BaseIndexingPolicy() {};

    /**
     * Associate a pointer to an entry to its physical counterpart.
     *
     * @param entry The entry pointer.
     * @param index An unique index for the entry.
     */
    void setEntry(ReplaceableEntry* entry, const uint64_t index);

    /**
     * Get an entry based on its set and way. All entries must have been set
     * already before calling this function.
     *
     * @param set The set of the desired entry.
     * @param way The way of the desired entry.
     * @return entry The entry pointer.
     */
    ReplaceableEntry* getEntry(const uint32_t set, const uint32_t way) const;

    /**
     * Generate the tag from the given address.
     *
     * @param addr The address to get the tag from.
     * @return The tag of the address.
     */
    virtual Addr extractTag(const Addr addr) const;

    /**
     * Find all possible entries for insertion and replacement of an address.
     * Should be called immediately before ReplacementPolicy's findVictim()
     * not to break cache resizing.
     *
     * @param addr The addr to a find possible entries for.
     * @return The possible entries.
     */
    virtual std::vector<ReplaceableEntry*> getPossibleEntries(const Addr addr)
                                                                    const = 0;

    /**
     * Regenerate an entry's address from its tag and assigned indexing bits.
     *
     * @param tag The tag bits.
     * @param entry The entry.
     * @return the entry's original address.
     */
    virtual Addr regenerateAddr(const Addr tag, const ReplaceableEntry* entry)
                                                                    const = 0;

    /**
     * The partition lookup cache for partitioned indexing. The PLC stores
     * mappings from address field to cache sectors.
     */
    PLC* plc;

    /**
     * Determine whether the plc is enabled or not.
     */
    bool isPLCEnabled;

    /**
     * The number of sets per sector.
     */
    unsigned sectSets;

    /**
     * Get the sets belonging to the sector.
     *
     * @param secId The sector ID.
     * @return the sets' entries belonging to the sector.
     */
    virtual std::vector<CacheBlk*> getSectorSets(int secId) const;

    /**
     * accessSector accesses the sector and update its replacement
     * data. It always returns true when the PLC is enabled.
     *
     * @param secId the sector id.
     * @return whether the access is successful or not.
     */
    virtual bool accessSector(int secId);

    /**
     * getVictimSector returns the victim sector according to PLC's
     * replacement policy. It returns -1 if there is no eviction.
     *
     * @return the victim sector id.
     */
    virtual int getVictimSector(Stats::Vector& contributions) const;
};

#endif //__MEM_CACHE_INDEXING_POLICIES_BASE_HH__
