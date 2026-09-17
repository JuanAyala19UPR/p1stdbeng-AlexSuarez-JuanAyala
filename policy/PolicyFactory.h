#ifndef P1DBENG_POLICY_FACTORY_H
#define P1DBENG_POLICY_FACTORY_H

#include "ReplacementPolicy.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace bufman {

// Maps policy names from conf.json to concrete ReplacementPolicy objects.
// Built-in names are registered by the factory itself; new policies (MRU,
// LFU, ...) register themselves with register_policy and become usable from
// conf.json without touching the factory's callers.
class PolicyFactory {
public:
    using Creator = std::function<std::unique_ptr<ReplacementPolicy>()>;

    static PolicyFactory& instance();

    void register_policy(const std::string& name, Creator creator);
    // Creates the policy registered under `name`; returns nullptr and fills
    // `error` (cleared on entry) for an unknown name.
    std::unique_ptr<ReplacementPolicy> create(const std::string& name,
                                              std::string& error) const;
    // Registered names, sorted.
    std::vector<std::string> known() const;

private:
    PolicyFactory();

    std::unordered_map<std::string, Creator> creators_;
};

}

#endif // P1DBENG_POLICY_FACTORY_H
