#ifndef P1DBENG_FIFO_POLICY_H
#define P1DBENG_FIFO_POLICY_H

#include "ReplacementPolicy.h"

#include <list>
#include <optional>
#include <unordered_map>
#include <vector>

namespace bufman {

// First-in, first-out: evict the candidate that entered the pool earliest,
// regardless of how often it was used since. The defining trait is what
// on_access does — nothing: FIFO orders by arrival, not recency, so a hit
// never refreshes a page's standing.
//
// Structure is the canonical arrival queue: a doubly linked list ordered
// oldest-first plus a hash map from frame to node. The map matters because
// the manager may evict any frame, including one buried mid-queue — with
// it, removal is O(1) on average from anywhere.
class FIFOPolicy final : public ReplacementPolicy {
public:
    void init(std::size_t pool_size) override;
    void on_access(std::size_t frame) override;
    void on_load(std::size_t frame) override;
    void on_remove(std::size_t frame) override;
    std::optional<std::size_t> pick_victim(
        const std::vector<std::size_t>& candidates) const override;

private:
    // Arrival order: front() is the oldest page and the next victim;
    // back() is the newest arrival. (The recency lists elsewhere in this
    // project are MRU-front — FIFO deliberately flips the convention.)
    std::list<std::size_t> queue_;

    // frame -> node in queue_. Presence means "tracked".
    std::unordered_map<std::size_t, std::list<std::size_t>::iterator> positions_;
};

}

#endif // P1DBENG_FIFO_POLICY_H
