//
// Created by alex on 9/22/26.
//
#include "MRUPolicy.h"

namespace bufman {
    void MRUPolicy::init(std::size_t pool_size) {

    }
    void MRUPolicy::on_access(std::size_t frame) {

    }
    void MRUPolicy::on_load(std::size_t frame) {

    }
    void MRUPolicy::on_remove(std::size_t frame) {

    }
    std::optional<std::size_t> MRUPolicy::pick_victim(
        const std::vector<std::size_t>& candidates) const{
        return std::nullopt;
    }


}