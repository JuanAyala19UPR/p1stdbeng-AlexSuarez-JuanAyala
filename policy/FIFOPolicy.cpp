//
// Created by alex on 9/22/26.
//
#include "FIFOPolicy.h"

#include <iterator>

namespace bufman {
    void FIFOPolicy::init(std::size_t pool_size) {

    }
    void FIFOPolicy::on_access(std::size_t frame) {


    }
    void FIFOPolicy::on_load(std::size_t frame) {


    }
    void FIFOPolicy::on_remove(std::size_t frame) {


    }
    std::optional<std::size_t> FIFOPolicy::pick_victim(
        const std::vector<std::size_t>& candidates) const {

        return std::nullopt;
    }
}



