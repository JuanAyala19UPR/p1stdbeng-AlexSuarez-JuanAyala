//
// Created by alex on 9/20/26.
//

#include "PartGenerator.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <random>
#include <string>

namespace bufman {

    namespace {

        constexpr std::array<const char*, 4> kMaterial = {
            "steel", "rubber", "aluminum", "lithium"
        };


    }

    std::vector<Part> generate_parts(std::size_t count, int first_part_id) {

        std::vector<Part> parts;
        parts.reserve(count);


        std::random_device random_device;
        std::mt19937 generator(random_device());

        for (std::size_t i=0; i < count; ++i) {
            Part part{};
            part.part_id = first_part_id + static_cast<int>(i);

            const std::string name = "P" + std::to_string(part.part_id);
            name.copy(part.part_name, std::min(name.size(), sizeof(part.part_name) - 1));

            std::uniform_real_distribution<float> dis_weight(0.0f, 99.9f);
            part.part_weight=dis_weight(generator);

            part.part_color=i%6;

            std::uniform_real_distribution<float> dis_price(0.0f, 40.0f);
            part.part_price=dis_price(generator);

            const char* material = kMaterial[i % kMaterial.size()];
            std::memcpy(part.part_material, material, std::min(sizeof(material), sizeof(part.part_material) - 1));
            parts.push_back(part);

        };


        return parts;
    }

}
