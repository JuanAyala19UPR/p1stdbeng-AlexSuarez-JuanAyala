//
// Created by alex on 9/22/26.
//
#include "LRUv2Policy.h"

namespace bufman {

    void LRUv2Policy::init(std::size_t pool_size) {

    }
    void LRUv2Policy::on_access(std::size_t frame) {

    }
    void LRUv2Policy::on_load(std::size_t frame) {

    }
    void LRUv2Policy::on_remove(std::size_t frame) {

    }
    std::optional<std::size_t> LRUv2Policy::pick_victim(
        const std::vector<std::size_t>& candidates) const{
            return std::nullopt;
    }
}

