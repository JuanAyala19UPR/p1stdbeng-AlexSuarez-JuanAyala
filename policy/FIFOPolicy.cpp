//
// Created by alex on 9/22/26.
//
#include "FIFOPolicy.h"

#include <algorithm>
#include <iterator>

namespace bufman {

void FIFOPolicy::init(std::size_t pool_size) {
    queue_.clear();
    positions_.clear();
    positions_.reserve(pool_size);
}

void FIFOPolicy::on_access(std::size_t frame) {
    // FIFO does not care about accesses.
    // A hit does not change arrival order.
}

void FIFOPolicy::on_load(std::size_t frame) {
    // Normally the manager calls on_remove before reusing a frame.
    // Still, avoid tracking the same frame twice.
    const auto found = positions_.find(frame);

    if (found != positions_.end()) {
        queue_.erase(found->second);
        positions_.erase(found);
    }

    // New arrivals go to the back.
    queue_.push_back(frame);

    auto it = queue_.end();
    --it;

    positions_[frame] = it;
}

void FIFOPolicy::on_remove(std::size_t frame) {
    const auto found = positions_.find(frame);

    if (found == positions_.end()) {
        return;
    }

    queue_.erase(found->second);
    positions_.erase(found);
}

std::optional<std::size_t> FIFOPolicy::pick_victim(
        const std::vector<std::size_t>& candidates) const {

    for (const std::size_t frame : queue_) {
        if (std::find(candidates.begin(),
                      candidates.end(),
                      frame) != candidates.end()) {
            return frame;
        }
    }

    return std::nullopt;
}

}