//
// Created by alex on 9/20/26.
//
#include "PartSerializer.h"

#include <algorithm>
#include <cassert>
#include <cstring>

namespace bufman {

// Packs one Person into `kRecordSize` bytes, field by field. We copy each
// field individually (rather than memcpy'ing the whole struct) because the
// compiler is free to insert padding between struct members; only this
// explicit, gap-free layout is guaranteed to match on disk.
void serialize_part(const Part& part, char* buffer, std::size_t max_len) {
    /*assert(buffer != nullptr);
    if (max_len < kRecordSize) {
        return;
    }*/
    /*
    std::size_t offset = 0;
    std::memcpy(buffer + offset, &part.part_id, sizeof(part.part_id));
    offset += sizeof(part.part_id);
    std::memcpy(buffer + offset, part.part_color, sizeof(part.part_color));
    offset += sizeof(part.part_color);
    std::memcpy(buffer + offset, &part.part_material, sizeof(part.part_material));
    offset += sizeof(part.part_material);
    std::memcpy(buffer + offset, part.part_name, sizeof(part.part_name));
    offset += sizeof(part.part_name);
    std::memcpy(buffer + offset, &part.part_price, sizeof(part.part_price));
    offset += sizeof(part.part_price);
    std::memcpy(buffer + offset, &part.part_price, sizeof(part.part_price));
    offset += sizeof(part.part_price);
    std::memcpy(buffer + offset, &part.part_weight, sizeof(part.part_weight));
    */
}

// Mirror image of serialize: walks the same fields in the same order at the
// same offsets, copying bytes out of the buffer and into a fresh Person.
bool deserialize_part(const char* buffer, std::size_t max_len, Part& part) {
    /*if (buffer == nullptr || max_len < kRecordSize) {
        return false;
    }/*
    /*
    person = Person{};
    std::size_t offset = 0;
    std::memcpy(&person.pid, buffer + offset, sizeof(person.pid));
    offset += sizeof(person.pid);
    std::memcpy(person.name, buffer + offset, sizeof(person.name));
    offset += sizeof(person.name);
    std::memcpy(&person.age, buffer + offset, sizeof(person.age));
    offset += sizeof(person.age);
    std::memcpy(person.city, buffer + offset, sizeof(person.city));
    return true;
    */
    return false;
}

// Zero-fills a block so every unused record slot reads back as pid == 0,
// which deserialize_block below treats as "end of data in this block".
void initialize_part_block(char* block) {
    /*
    assert(block != nullptr);
    std::memset(block, 0, kPartBlockSize);
    */
}

// Lays out up to kRecordsPerBlock records back to back: record i starts at
// byte i * kRecordSize. Any leftover slots stay zeroed by initialize_block.
std::size_t serialize_part_block(const std::vector<Part>& parts, char* block) {
    /*
    assert(block != nullptr);
    initialize_block(block);
    const std::size_t count = std::min(parts.size(), kPartsPerBlock);
    for (std::size_t i = 0; i < count; ++i) {
        serialize(persons[i], block + i * kPartRecordSize, kPartRecordSize);
    }
    return count;
    */
    return 0;
}

// Reads records out of a block in the same fixed-slot order, stopping at the
// first pid == 0 slot (see initialize_block) since that marks unused space.
std::vector<Part> deserialize_part_block(const char* block) {
    /*
    std::vector<Person> parts;
    if (block == nullptr) {
        return parts;
    }

    for (std::size_t i = 0; i < kPartsPerBlock; ++i) {
        Part part{};
        if (!deserialize(block + i * kPartRecordSize, kPartRecordSize, part) || part.part_id == 0) {
            break;
        }
        parts.push_back(part);
    }
    return parts;
    */

    return std::vector<Part>();
}

std::size_t part_record_count(const char* block) {
    /*
    if (block == nullptr) {
        return 0;
    }
    for (std::size_t i = 0; i < kPartsPerBlock; ++i) {
        Part part{};
        if (!deserialize(block + i * kPartRecordSize, kPartRecordSize, part) || part.part_id == 0) {
            return i;
        }
    }
    return kPartsPerBlock;
    */
    return 0;
}
std::optional<std::size_t> first_free_part_slot(const char* block) {
    /*
    if (block == nullptr) {
        return std::nullopt;
    }
    for (std::size_t i = 0; i < kRecordsPerBlock; ++i) {
        Person person{};
        if (!deserialize(block + i * kRecordSize, kRecordSize, person) || person.pid == 0) {
            return i;
        }
    }
    return std::nullopt;
    */
    return std::nullopt;
}


bool get_part_record(const char* block, std::size_t slot, Part& part) {
    /*
    if (block == nullptr || slot >= kPartsPerBlock) {
        return false;
    }
    if (!deserialize(block + slot * kPartRecordSize, kPartRecordSize, part)) {
        return false;
    }
    return part.part_id != 0;
    */
    return false;
}

bool put_part_record(char* block, std::size_t slot, const Part& part) {
    /*
    if (block == nullptr || slot >= kPartsPerBlock || part.part_id == 0) {
        return false;
    }
    serialize(part, block + slot * kPartRecordSize, kPartRecordSize);
    return true;
    */
    return false;
}

}
