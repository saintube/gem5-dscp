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
 * Definitions of a common framework for indexing policies.
 */

#include "mem/cache/tags/indexing_policies/base.hh"

#include <cstdlib>

#include "base/intmath.hh"
#include "base/logging.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"

BaseIndexingPolicy::BaseIndexingPolicy(const Params *p)
    : SimObject(p), assoc(p->assoc),
      numSets(p->size / (p->entry_size * assoc)),
      setShift(floorLog2(p->entry_size)), setMask(numSets - 1), sets(numSets),
      tagShift(setShift + floorLog2(numSets)), plc_size(32)
      // TODO: set an approriate shift for the plc
{
    fatal_if(!isPowerOf2(numSets), "# of sets must be non-zero and a power " \
             "of 2");
    fatal_if(assoc <= 0, "associativity must be greater than zero");

    // Make space for the entries
    for (uint32_t i = 0; i < numSets; ++i) {
        sets[i].resize(assoc);
    }

    // TODO: initialize the plc
    fatal_if(plc_size < 0, "plc size must be no less than zero");
    fatal_if(tagShift < 0, "tagShift must be no less than zero");
    plc = new PLC((unsigned)plc_size, (unsigned)tagShift);
}

ReplaceableEntry*
BaseIndexingPolicy::getEntry(const uint32_t set, const uint32_t way) const
{
    return sets[set][way];
}

void
BaseIndexingPolicy::setEntry(ReplaceableEntry* entry, const uint64_t index)
{
    // Calculate set and way from entry index
    const std::lldiv_t div_result = std::div((long long)index, assoc);
    const uint32_t set = div_result.quot;
    const uint32_t way = div_result.rem;

    // Sanity check
    assert(set < numSets);

    // Assign a free pointer
    sets[set][way] = entry;

    // Inform the entry its position
    entry->setPosition(set, way);
}

Addr
BaseIndexingPolicy::extractTag(const Addr addr) const
{
    return (addr >> tagShift);
}

std::vector<CacheBlk*>
BaseIndexingPolicy::getSectorSets(int secId) const
{
    std::vector<CacheBlk*> entries;
    return entries;
}

bool
BaseIndexingPolicy::accessSector(int secId)
{
    // the PLC is disabled by default
    return false;
}

int
BaseIndexingPolicy::getVictimSector(Stats::Vector& contributions) const
{
    // the PLC is disabled by default
    return -1;
}

PLC::PLC(unsigned psize, unsigned shift)
{
    // possibly get reset by initSector()
    capacity = psize;
    tagShift = shift;
    tagMask = (1 << (floorLog2(psize) + 1)) - 1;
};

bool
PLC::isFull()
{
    return m.size() >= capacity;
}

void
PLC::initSectors(unsigned pSects)
{
    pSectors = pSects;
    count = 0;
    capacity = pSectors << 4;
    tagMask = (1 << (floorLog2(capacity) + 0)) - 1;
    // reset all entries
    m.erase(m.begin(), m.end());
    // only resize sc bits when the PLC is enabled
    sc.resize(pSects);
    sc.erase(sc.begin(), sc.end());
}

int
PLC::getSector(const Addr addr)
{
    /**
     * FIXME: implement the real lookup
     */
    // return ((addr >> tagShift) % pSectors);
    unsigned addrField = ((addr >> tagShift) & tagMask);
    if (m.count(addrField)) {
        return m[addrField];
    }
    return -1;
}

bool
PLC::setPLCEntry(const Addr addr, int secId)
{
    unsigned addrField = ((addr >> tagShift) & tagMask);
    if (secId < 0) {
        return false;
    }
    m[addrField] = secId;
    sc[secId] = true;
    return true;
}

bool
PLC::deletePLCEntry(const Addr addr)
{
    unsigned addrField = ((addr >> tagShift) & tagMask);
    m.erase(addrField);
    return true;
}

bool
PLC::deletePLCEntry(int secId)
{
    if (secId < 0)
        return false;
    bool changed = false;
    for (auto iter : m) {
        if (iter.second == secId) {
            m.erase(iter.first);
            changed = true;
        }
    }
    sc[secId] = false;
    return changed;
}

void
PLC::setSC(int secId, bool value)
{
    sc[secId] = value;
}

bool
PLC::getSC(int secId)
{
    return sc[secId];
}
