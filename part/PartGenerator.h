#ifndef P1DBENG_PART_GENERATOR_H
#define P1DBENG_PART_GENERATOR_H

#include "Part.h"

#include <cstddef>
#include <vector>

namespace bufman {

// Generates `count` synthetic Part records for bulk-insert testing, with
// part ids assigned sequentially starting at `first_part_id`.
std::vector<Part> generate_parts(std::size_t count, int first_part_id);

}

#endif // P1DBENG_PART_GENERATOR_H
