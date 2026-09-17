#ifndef P1DBENG_LRU_V2_POLICY_H
#define P1DBENG_LRU_V2_POLICY_H

#include "ReplacementPolicy.h"

#include <list>
#include <optional>
#include <unordered_map>
#include <vector>

namespace bufman {

// LRU written in the canonical textbook form: a doubly linked list keeps
// the recency order (MRU at front, LRU at back) and a hash map takes a
// frame to its node in that list. Presence in the map means "tracked".
// Together they give O(1) average lookup, promotion, insertion, and
// eviction — O(1) on average because hashing is, worst case O(n).
//
// Behaviorally identical to LRUPolicy: this class exists as the readable
// reference for the list + hash-map idiom, while LRUPolicy is the
// vector-indexed variant that exploits frame ids being dense. Storing
// list iterators as map values is safe: a std::list iterator stays valid
// across insertions and erasures that do not touch its own element.
class LRUv2Policy final : public ReplacementPolicy {
public:
    void init(std::size_t pool_size) override;
    void on_access(std::size_t frame) override;
    void on_load(std::size_t frame) override;
    void on_remove(std::size_t frame) override;
    std::optional<std::size_t> pick_victim(
        const std::vector<std::size_t>& candidates) const override;

private:
    void touch(std::size_t frame);

    // MRU at front, LRU at back.
    std::list<std::size_t> order_;

    // frame -> node in order_. order_ and nodes_ are kept in lockstep:
    // every list entry has a map entry and vice versa.
    std::unordered_map<std::size_t, std::list<std::size_t>::iterator> nodes_;
};

}

#endif // P1DBENG_LRU_V2_POLICY_H
