//
// Created by alex on 9/20/26.
//

#include "PartGenerator.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <string>

namespace bufman {
    /*
    namespace {

        constexpr std::array<const char*, 4> kMaterial = {
            "steel", "rubber", "aluminum", "lithium"
        };


    }
    */
    std::vector<Part> generate_parts(std::size_t count, int first_pid) {
        /*
        std::vector<Part> parts;
        parts.reserve(count);
        */
        /*
        std::random_device random_device;
        std::mt19937 generator(random_device());

        for (std::size_t i =
        parts.reserve(co0; i < count; ++i) {
            Part part{};
            part.part_id = first_pid + static_cast<int>(i);

            const std::string name = "P" + std::to_string(person.pid);
            name.copy(part.part_name, std::min(name.size(), sizeof(person.name) - 1));

            std::uniform_real_distribution<float> dis_weight(0.0f, 99.9f);
            const float weight=dis_weight(generator);
            std::memcpy(part.part_weight, &weight, sizeof(weight));

            const int color=i%6;
            std::memcpy(part.part_color, &color, sizeof(int));

            std::uniform_real_distribution<float> dis_price(0.0f, 40.0f);
            const float price=dis_price(generator);
            std::memcpy(part.part_price, &price, sizeof(price));

            const char* material = kMaterial[i % kMaterial.size()];
            std::memcpy(part.part_material, material, std::min(material.size(), sizeof(part.material) - 1));
                parts.push_back(part);constexpr st
    "steel",
};
        }
        */
        std::vector<Part> parts;
        return  parts;
    }

}
