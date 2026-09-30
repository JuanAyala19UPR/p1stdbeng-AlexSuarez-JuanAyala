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
    assert(buffer!=nullptr);
    if (max_len < kPartRecordSize) {
        return ;

    }
    std::size_t offset=0;
    std::memcpy(buffer+offset, &part.part_id, sizeof(part.part_id));
    offset+=sizeof(part.part_id);
    std::memcpy(buffer+offset, part.part_name, sizeof(part.part_name));
    offset+=sizeof(part.part_name);
    std::memcpy(buffer+offset, &part.part_weight, sizeof(part.part_weight));
    offset+=sizeof(part.part_weight);
    std::memcpy(buffer+offset, &part.part_color, sizeof(part.part_color));
    offset+=sizeof(part.part_color);
    std::memcpy(buffer+offset, &part.part_price, sizeof(part.part_price));
    offset+=sizeof(part.part_price);
    std::memcpy(buffer+offset, part.part_material, sizeof(part.part_material));
}

// Mirror image of serialize: walks the same fields in the same order at the
// same offsets, copying bytes out of the buffer and into a fresh Person.
bool deserialize_part(const char* buffer, std::size_t max_len, Part& part) {
    if (buffer==nullptr || max_len<kPartRecordSize) {
        return false;
    }

    std::size_t offset=0;
    part=Part{};
    std::memcpy(&part.part_id, buffer+offset, sizeof(part.part_id));
    offset+=sizeof(part.part_id);
    std::memcpy(part.part_name, buffer+offset, sizeof(part.part_name));
    offset+=sizeof(part.part_name);
    std::memcpy(&part.part_weight, buffer+offset, sizeof(part.part_weight));
    offset+=sizeof(part.part_weight);
    std::memcpy(&part.part_color, buffer+offset, sizeof(part.part_color));
    offset+=sizeof(part.part_weight);
    std::memcpy(&part.part_price, buffer+offset, sizeof(part.part_price));
    offset+=sizeof(part.part_price);
    std::memcpy(part.part_material, buffer+offset,sizeof(part.part_material));
    return true;

}

// Zero-fills a block so every unused record slot reads back as pid == 0,
// which deserialize_block below treats as "end of data in this block".
void initialize_part_block(char* block) {
    assert(block!=nullptr);
    std::memset(block, 0, kPartBlockSize);
}

// Lays out up to kRecordsPerBlock records back to back: record i starts at
// byte i * kRecordSize. Any leftover slots stay zeroed by initialize_block.
std::size_t serialize_part_block(const std::vector<Part>& parts, char* block) {
    assert(block!=nullptr);
    initialize_part_block(block);
    const std::size_t count=std::min(parts.size(),kPartsPerBlock);
    for (std::size_t i=0; i<count; i++) {
        serialize_part(parts[i], block+ i *kPartRecordSize, kPartRecordSize);
    }
    return count;

}

// Reads records out of a block in the same fixed-slot order, stopping at the
// first pid == 0 slot (see initialize_block) since that marks unused space.
std::vector<Part> deserialize_part_block(const char* block) {
    std::vector<Part> parts;
    if (block==nullptr) {
        return parts;
    }
    for (std::size_t i=0;i<kPartsPerBlock;i++) {
        Part part{};
        if (!deserialize_part(block+i * kPartRecordSize,kPartRecordSize, part) || part.part_id==0 ) {
            break;
        }
        parts.push_back(part);
    }
    return parts;

}

std::size_t part_record_count(const char* block) {
    if (block==nullptr) {
        return 0;
    }
    Part part{};
    for (std::size_t i=0;i<kPartsPerBlock;i++) {
        if (!deserialize_part(block+i*kPartRecordSize,kPartRecordSize,part) || part.part_id==0) {
            //Si el tamaño maximo a ocupar es mayor tamaño del record significa que cuenta hasta ahi.Por ahora max_len=partRecordSize pero cuando trabajemos con varchar se aplicara.
            return i;
        }
    }
    return kPartsPerBlock;

}
std::optional<std::size_t> first_free_part_slot(const char* block) {
    if (block==nullptr) {
        return std::nullopt;
    }
    Part part{};
    for (std::size_t i=0;i<kPartsPerBlock;i++ ) {
        if (!deserialize_part(block+i*kPartRecordSize,kPartRecordSize,part) || part.part_id==0) {
            return i;
        }
    }
    return std::nullopt;

}


bool get_part_record(const char* block, std::size_t slot, Part& part) {
    if (block==nullptr || slot>=kPartsPerBlock) {
        return false;
    }
    if (!deserialize_part(block+slot*kPartRecordSize, kPartsPerBlock,part)) {
        return false;
    }
    return part.part_id!=0;
}

bool put_part_record(char* block, std::size_t slot, const Part& part) {
    if (block==nullptr || slot>=kPartsPerBlock || part.part_id==0) {
        return false;
    }
    serialize_part(part,block+slot*kPartRecordSize,kPartsPerBlock);
    return true;
}

}
