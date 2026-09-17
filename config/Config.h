#ifndef P1DBENG_CONFIG_H
#define P1DBENG_CONFIG_H

#include <cstddef>
#include <iosfwd>
#include <string>

namespace bufman {

struct Config {
    std::string policy;         // resolved through the PolicyFactory
    std::size_t pool_size = 0;  // validated to be >= 1
};

// Reads a flat JSON object with two entries — "policy" (a quoted string)
// and "pool_size" (an integer >= 1) — from `path`. Both entries are
// required; duplicate keys, trailing commas, trailing garbage, and
// non-integer pool sizes are rejected. Unknown keys are skipped with a
// warning on the diagnostics stream. Follows the house error convention:
// `error` is cleared on entry and filled only on failure.
bool load_config(const std::string& path, Config& out,
                 std::ostream& diagnostics, std::string& error);

}

#endif // P1DBENG_CONFIG_H
