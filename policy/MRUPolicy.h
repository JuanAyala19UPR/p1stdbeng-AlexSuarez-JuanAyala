#ifndef P1DBENG_MRU_POLICY_H
#define P1DBENG_MRU_POLICY_H

#include "ReplacementPolicy.h"

#include <list>
#include <optional>
#include <vector>

namespace bufman {

// Most-recently-used: among the candidates, evict the frame that was
// accessed last, not first. The bookkeeping is a verbatim mirror of
// LRUPolicy (the same recency list, updated on pin); only pick_victim
// differs — it walks from the MRU end of the list.
//
// Why MRU exists: cyclic scans over a file larger than the pool. Under
// LRU every cycle evicts pages just before the next cycle needs them, so
// every access is a miss. MRU evicts the page just used, which the next
// cycle needs last, so the rest of the cycle stays cached. The check
// suite demonstrates this with a two-cycle scan over three blocks in a
// two-frame pool: mru gets hits, lru gets none.
class MRUPolicy final : public ReplacementPolicy {
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
    std::vector<std::list<std::size_t>::iterator> positions_;  // O(1) entry per frame
};

}

#endif // P1DBENG_MRU_POLICY_H
