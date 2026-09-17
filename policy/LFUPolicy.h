#ifndef P1DBENG_LFU_POLICY_H
#define P1DBENG_LFU_POLICY_H

#include "ReplacementPolicy.h"

#include <list>
#include <optional>
#include <unordered_map>
#include <vector>

namespace bufman {

// Least-frequently-used: among the candidates, evict the frame whose page
// has the fewest recorded accesses; count ties are broken by recency
// (least recently used first). The canonical O(1)-average design: one
// doubly linked list per frequency count, plus a hash map from frame to
// its count and node.
//
// Two honest limitations, both inherited from the manager/policy contract:
// counts reset when a page leaves the frame (on_remove) — there is no
// cross-eviction memory, so this is LFU per residency, not per page; and
// counts never decay, so a page that was hot once stays heavy until it is
// evicted. Aging (halving counts periodically) would be the extension.
class LFUPolicy final : public ReplacementPolicy {
public:
    void init(std::size_t pool_size) override;
    void on_access(std::size_t frame) override;
    void on_load(std::size_t frame) override;
    void on_remove(std::size_t frame) override;
    std::optional<std::size_t> pick_victim(
        const std::vector<std::size_t>& candidates) const override;

private:
    // Count -> frames with that count, MRU at front, LRU at back. Empty
    // buckets are erased immediately, so the map holds only live counts.
    std::unordered_map<std::size_t, std::list<std::size_t>> buckets_;

    struct Slot {
        std::size_t count = 0;
        std::list<std::size_t>::iterator pos;
    };
    // frame -> {count, node in its bucket}. Presence means "tracked".
    std::unordered_map<std::size_t, Slot> slots_;

    // Count of the least-frequent tracked page; 0 while nothing is tracked.
    std::size_t min_freq_ = 0;
};

}

#endif // P1DBENG_LFU_POLICY_H
