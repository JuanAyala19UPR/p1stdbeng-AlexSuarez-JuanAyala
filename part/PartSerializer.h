#ifndef P1DBENG_PART_SERIALIZER_H
#define P1DBENG_PART_SERIALIZER_H

#include "Part.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace bufman {

// Gap-free on-disk layout: part_id (4) + part_name (10) + part_weight (4)
// + part_color (4) + part_price (4) + part_material (10) = 36 bytes, fields
// back to back in struct order. The in-memory struct is NOT written as-is
// (it contains padding), so only these explicit copies define the format.
constexpr std::size_t kPartRecordSize = sizeof(int) + 10 + sizeof(float) +
                                        sizeof(int) + sizeof(float) + 10;
constexpr std::size_t kPartBlockSize = 4096;
constexpr std::size_t kPartsPerBlock = kPartBlockSize / kPartRecordSize;

static_assert(sizeof(int) == 4 && sizeof(float) == 4,
              "the on-disk format assumes 32-bit int and float");

// Same conventions as the person format: part_id == 0 is the free-slot
// sentinel, so a Part with part_id 0 can never be stored, and a zeroed
// block reads back as empty. Valid records have part_id > 0,
// part_weight >= 0, part_price >= 0, and part_color in [0, 5].
void serialize_part(const Part& part, char* buffer, std::size_t max_len);
bool deserialize_part(const char* buffer, std::size_t max_len, Part& part);
void initialize_part_block(char* block);
std::size_t serialize_part_block(const std::vector<Part>& parts, char* block);
std::vector<Part> deserialize_part_block(const char* block);

// Slot-level helpers over any block-sized buffer (e.g. a DataFrame's).
std::size_t part_record_count(const char* block);
std::optional<std::size_t> first_free_part_slot(const char* block);
bool get_part_record(const char* block, std::size_t slot, Part& part);
bool put_part_record(char* block, std::size_t slot, const Part& part);

}

#endif // P1DBENG_PART_SERIALIZER_H
