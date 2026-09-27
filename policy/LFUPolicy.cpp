//
// Created by alex on 9/22/26.
//

#include "LFUPolicy.h"

namespace bufman {
    void LFUPolicy::init(std::size_t pool_size) {

    }
    void LFUPolicy::on_access(std::size_t frame) {


    }
    void LFUPolicy::on_load(std::size_t frame) {


    }
    void LFUPolicy::on_remove(std::size_t frame) {


    }
    std::optional<std::size_t> LFUPolicy::pick_victim(
        const std::vector<std::size_t>& candidates) const {

            return std::nullopt;
    }


}