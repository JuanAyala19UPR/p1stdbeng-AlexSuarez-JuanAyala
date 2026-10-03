//
// Created by alex on 9/22/26.
//
#include "LRUv2Policy.h"

    namespace bufman {

        void LRUv2Policy::init(std::size_t pool_size) {
        order_.clear();
        nodes_.clear();
        nodes_.reserve(pool_size);
    }

    void LRUv2Policy::touch(std::size_t frame) {
        // If the frame is already being tracked,
        // remove it from its current position.
        auto it = nodes_.find(frame);

        if (it != nodes_.end()) {
            order_.erase(it->second);
            nodes_.erase(it);
        }

        // The accessed/loaded frame becomes the MRU.
        order_.push_front(frame);
        nodes_[frame] = order_.begin();
    }

    void LRUv2Policy::on_access(std::size_t frame) {
        touch(frame);
    }

    void LRUv2Policy::on_load(std::size_t frame) {
        touch(frame);
    }

    void LRUv2Policy::on_remove(std::size_t frame) {
        auto it = nodes_.find(frame);

        if (it != nodes_.end()) {
            order_.erase(it->second);
            nodes_.erase(it);
        }
    }
    std::optional<std::size_t> LRUv2Policy::pick_victim(
        const std::vector<std::size_t>& candidates) const {

    // order_: MRU at front, LRU at back.
    // Therefore search from the back toward the front.
    for (auto it = order_.rbegin(); it != order_.rend(); ++it) {
        if (std::find(candidates.begin(),
                      candidates.end(),
                      *it) != candidates.end()) {
            return *it;
        }
    }

    return std::nullopt;
}
}

