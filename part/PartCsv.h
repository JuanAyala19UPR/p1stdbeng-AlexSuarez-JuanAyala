#ifndef P1DBENG_PART_CSV_H
#define P1DBENG_PART_CSV_H

#include "Part.h"

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

namespace bufman {

struct PartLoadResult {
    std::vector<Part> parts;      // valid rows, in file order
    std::size_t skipped = 0;      // rejected rows (and blank lines)
};

// Loads a CSV of parts: exactly six comma-separated fields per row —
// part_id, part_name, part_weight, part_color, part_price, part_material —
// mirroring the person CSV loader's rules. Invalid rows are reported to the
// diagnostics stream and skipped, never fatal.
PartLoadResult load_parts(const std::string& path, std::ostream& diagnostics);

}

#endif // P1DBENG_PART_CSV_H
