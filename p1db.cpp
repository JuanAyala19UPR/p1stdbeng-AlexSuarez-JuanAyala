#include "file/BlockFile.h"
#include "bufpool/BufferManager.h"
#include "config/Config.h"
#include "part/PartCsv.h"
#include "part/PartGenerator.h"
#include "part/PartSerializer.h"
#include "policy/PolicyFactory.h"
#include "person/PersonCsv.h"
#include "person/PersonGenerator.h"
#include "person/PersonSerializer.h"

#include <array>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <unistd.h>

namespace {

bool parse_uint64(const char* text, std::uint64_t& value) {
    const char* end = text + std::char_traits<char>::length(text);
    const auto result = std::from_chars(text, end, value);
    return result.ec == std::errc{} && result.ptr == end;
}

// ---------------------------------------------------------------------------
// Table adapters: everything the commands need to know about a record type,
// so each command is written once and dispatched over person/part.
// ---------------------------------------------------------------------------

struct PersonOps {
    using Record = Person;
    using LoadResult = bufman::LoadResult;

    static constexpr const char* kNoun = "record(s)";

    static LoadResult load(const std::string& path, std::ostream& diagnostics) {
        return bufman::load(path, diagnostics);
    }
    static const std::vector<Record>& records(const LoadResult& result) {
        return result.people;
    }
    static std::vector<Record> generate(std::size_t count, int first_id) {
        return bufman::generate(count, first_id);
    }
    static std::vector<Record> deserialize_block(const char* block) {
        return bufman::deserialize_block(block);
    }
    static bool put_record(char* block, std::size_t slot, const Record& record) {
        return bufman::put_record(block, slot, record);
    }
    static std::size_t record_count(const char* block) {
        return bufman::record_count(block);
    }
    static std::uint64_t record_count_file(bufman::File& file, std::string& error) {
        return bufman::record_count(file, error);
    }
    static std::size_t records_per_block() {
        return bufman::kRecordsPerBlock;
    }
    static void print(const Record& person, std::size_t index) {
        std::cout << "  slot " << index << ": pid=" << person.pid
                  << ", name=" << person.name
                  << ", age=" << person.age
                  << ", city=" << person.city << '\n';
    }
};

struct PartOps {
    using Record = Part;
    using LoadResult = bufman::PartLoadResult;

    static constexpr const char* kNoun = "part(s)";

    static LoadResult load(const std::string& path, std::ostream& diagnostics) {
        return bufman::load_parts(path, diagnostics);
    }
    static const std::vector<Record>& records(const LoadResult& result) {
        return result.parts;
    }
    static std::vector<Record> generate(std::size_t count, int first_id) {
        return bufman::generate_parts(count, first_id);
    }
    static std::vector<Record> deserialize_block(const char* block) {
        return bufman::deserialize_part_block(block);
    }
    static bool put_record(char* block, std::size_t slot, const Record& record) {
        return bufman::put_part_record(block, slot, record);
    }
    static std::size_t record_count(const char* block) {
        return bufman::part_record_count(block);
    }
    // Parts-aware counterpart of BlockFile::record_count: every block except
    // possibly the last is full, so only the last block's occupancy is read.
    static std::uint64_t record_count_file(bufman::File& file, std::string& error) {
        const std::uint64_t blocks = bufman::block_count(file, error);
        if (!error.empty() || blocks == 0) {
            return 0;
        }
        std::array<char, bufman::kPartBlockSize> block{};
        if (!bufman::read_block(file, blocks - 1, block, error)) {
            return 0;
        }
        return (blocks - 1) * bufman::kPartsPerBlock +
               bufman::part_record_count(block.data());
    }
    static std::size_t records_per_block() {
        return bufman::kPartsPerBlock;
    }
    static void print(const Record& part, std::size_t index) {
        std::cout << "  slot " << index << ": part_id=" << part.part_id
                  << ", name=" << part.part_name
                  << ", weight=" << part.part_weight
                  << ", color=" << part.part_color
                  << ", price=" << part.part_price
                  << ", material=" << part.part_material << '\n';
    }
};

// ---------------------------------------------------------------------------
// Session state: one shared manager plus a cache of files opened through it.
// Keeping files open across commands is what makes the pool useful: pages
// stay cached between commands, so a read after a scan hits the pool.
// ---------------------------------------------------------------------------

class FileCache {
public:
    // Returns a handle for `path`, opening it through `mgr` on first use.
    // `want_write` selects the open mode; a file already open read-only
    // refuses write access (the manager has no per-file close, so the
    // remedy is to quit and start a new session).
    bufman::File* get(bufman::BufferManager& mgr, const std::string& path,
                      bool want_write, std::string& error) {
        const auto found = entries_.find(path);
        if (found != entries_.end()) {
            if (want_write && !found->second.writable) {
                error = "file is open read-only; quit and restart to modify it";
                return nullptr;
            }
            return found->second.file;
        }
        bufman::File* file = nullptr;
        const bool opened = want_write ? mgr.open_file(path, file, error)
                                       : mgr.open_file_read(path, file, error);
        if (!opened) {
            return nullptr;
        }
        entries_[path] = Entry{file, want_write};
        return file;
    }

private:
    struct Entry {
        bufman::File* file = nullptr;
        bool writable = false;
    };

    std::unordered_map<std::string, Entry> entries_;
};

// ---------------------------------------------------------------------------
// Command implementations (one per command, templated over the table).
// ---------------------------------------------------------------------------

// Appends records through the buffer pool: fill the last block's free slots
// first, then allocate new pages for the rest. Every touched frame is
// unpinned on every path — the pool is shared by the whole session, so a
// leaked pin would slowly wedge it.
template <typename Ops>
bool append_records(bufman::BufferManager& mgr, bufman::File& file,
                    const std::vector<typename Ops::Record>& records,
                    std::string& error) {
    std::uint64_t blocks = bufman::block_count(file, error);
    if (!error.empty()) {
        return false;
    }

    std::size_t next = 0;
    if (blocks > 0) {
        std::size_t f = 0;
        if (!mgr.pin(file, blocks - 1, f, error)) {
            return false;
        }
        std::size_t slot = Ops::record_count(mgr.frame(f).buffer);
        bool touched = false;
        bool ok = true;
        while (slot < Ops::records_per_block() && next < records.size()) {
            if (!Ops::put_record(mgr.frame(f).buffer, slot++, records[next++])) {
                error = "put_record failed while filling the last block";
                ok = false;
                break;
            }
            touched = true;
        }
        std::string unpin_error;
        mgr.unpin(f, touched, unpin_error);
        if (!ok) {
            return false;
        }
        if (!unpin_error.empty()) {
            error = unpin_error;
            return false;
        }
    }

    while (next < records.size()) {
        std::size_t f = 0;
        if (!mgr.alloc_page(file, blocks, f, error)) {
            return false;
        }
        ++blocks;
        std::size_t slot = 0;
        bool ok = true;
        while (slot < Ops::records_per_block() && next < records.size()) {
            if (!Ops::put_record(mgr.frame(f).buffer, slot++, records[next++])) {
                error = "put_record failed while filling a new page";
                ok = false;
                break;
            }
        }
        std::string unpin_error;
        mgr.unpin(f, true, unpin_error);
        if (!ok) {
            return false;
        }
        if (!unpin_error.empty()) {
            error = unpin_error;
            return false;
        }
    }
    return true;
}

template <typename Ops>
void cmd_append(bufman::BufferManager& mgr, FileCache& cache,
                const std::vector<std::string>& args) {
    const auto loaded = Ops::load(args[2], std::cerr);
    const auto& records = Ops::records(loaded);
    if (records.empty()) {
        if (loaded.skipped != 0) {
            std::cerr << "no valid " << Ops::kNoun << " to append\n";
        }
        return;
    }
    std::string error;
    bufman::File* file = cache.get(mgr, args[3], true, error);
    if (file == nullptr) {
        std::cerr << error << '\n';
        return;
    }
    if (!append_records<Ops>(mgr, *file, records, error)) {
        std::cerr << error << '\n';
        return;
    }
    // Write the new pages to disk now: later commands size things up via
    // the file on disk, and a session keeps running after this command.
    // The pages stay resident (and clean), so reads still hit the pool.
    std::string flush_error;
    if (!mgr.flush_all(flush_error)) {
        std::cerr << flush_error << '\n';
    }
    std::cout << "appended " << records.size() << " " << Ops::kNoun
              << ", skipped " << loaded.skipped << " row(s)\n";
}

template <typename Ops>
void cmd_bulk(bufman::BufferManager& mgr, FileCache& cache,
              const std::vector<std::string>& args) {
    std::uint64_t count = 0;
    if (!parse_uint64(args[2].c_str(), count)) {
        std::cerr << "count must be a nonnegative integer\n";
        return;
    }
    if (count == 0) {
        std::cout << "nothing to insert\n";
        return;
    }
    std::string error;
    bufman::File* file = cache.get(mgr, args[3], true, error);
    if (file == nullptr) {
        std::cerr << error << '\n';
        return;
    }
    const std::uint64_t existing = Ops::record_count_file(*file, error);
    if (!error.empty()) {
        std::cerr << error << '\n';
        return;
    }
    if (existing > static_cast<std::uint64_t>(std::numeric_limits<int>::max() - 1)) {
        std::cerr << "too many existing " << Ops::kNoun << " to continue numbering\n";
        return;
    }
    const auto records = Ops::generate(count, static_cast<int>(existing) + 1);
    if (!append_records<Ops>(mgr, *file, records, error)) {
        std::cerr << error << '\n';
        return;
    }
    std::string flush_error;
    if (!mgr.flush_all(flush_error)) {
        std::cerr << flush_error << '\n';
    }
    std::cout << "appended " << records.size() << " generated " << Ops::kNoun << '\n';
}

template <typename Ops>
void cmd_read(bufman::BufferManager& mgr, FileCache& cache,
              const std::vector<std::string>& args) {
    std::uint64_t block_number = 0;
    if (!parse_uint64(args[3].c_str(), block_number)) {
        std::cerr << "block number must be a nonnegative integer\n";
        return;
    }
    std::string error;
    bufman::File* file = cache.get(mgr, args[2], false, error);
    if (file == nullptr) {
        std::cerr << error << '\n';
        return;
    }
    std::size_t f = 0;
    if (!mgr.pin(*file, block_number, f, error)) {
        std::cerr << error << '\n';
        return;
    }
    const auto records = Ops::deserialize_block(mgr.frame(f).buffer);
    std::cout << "block " << block_number << ": " << records.size() << " "
              << Ops::kNoun << '\n';
    for (std::size_t i = 0; i < records.size(); ++i) {
        Ops::print(records[i], i);
    }
    mgr.unpin(f, false, error);
}

// Reads each requested block in command-line order, duplicates included.
// A block that fails (bad number, beyond EOF) prints a message and the loop
// continues.
template <typename Ops>
void cmd_seek(bufman::BufferManager& mgr, FileCache& cache,
              const std::vector<std::string>& args) {
    std::string error;
    bufman::File* file = cache.get(mgr, args[2], false, error);
    if (file == nullptr) {
        std::cerr << error << '\n';
        return;
    }
    for (std::size_t i = 3; i < args.size(); ++i) {
        std::uint64_t block_number = 0;
        if (!parse_uint64(args[i].c_str(), block_number)) {
            std::cerr << "block " << args[i] << ": not a nonnegative integer\n";
            continue;
        }
        const bool hit = mgr.resident(*file, block_number);
        std::size_t f = 0;
        if (!mgr.pin(*file, block_number, f, error)) {
            std::cerr << "block " << block_number << ": " << error << '\n';
            continue;
        }
        const auto records = Ops::deserialize_block(mgr.frame(f).buffer);
        std::cout << "block " << block_number;
        if (hit) {
            std::cout << " (pool hit)";
        }
        std::cout << ": " << records.size() << " " << Ops::kNoun << '\n';
        for (std::size_t p = 0; p < records.size(); ++p) {
            Ops::print(records[p], p);
        }
        mgr.unpin(f, false, error);
    }
}

template <typename Ops>
void cmd_scan(bufman::BufferManager& mgr, FileCache& cache,
              const std::vector<std::string>& args) {
    std::string error;
    bufman::File* file = cache.get(mgr, args[2], false, error);
    if (file == nullptr) {
        std::cerr << error << '\n';
        return;
    }
    const std::uint64_t blocks = bufman::block_count(*file, error);
    if (!error.empty()) {
        std::cerr << error << '\n';
        return;
    }
    std::uint64_t total_records = 0;
    for (std::uint64_t block_number = 0; block_number < blocks; ++block_number) {
        std::size_t f = 0;
        if (!mgr.pin(*file, block_number, f, error)) {
            std::cerr << error << '\n';
            return;
        }
        const auto records = Ops::deserialize_block(mgr.frame(f).buffer);
        std::cout << "block " << block_number << ": " << records.size() << " "
                  << Ops::kNoun << '\n';
        for (std::size_t i = 0; i < records.size(); ++i) {
            Ops::print(records[i], i);
        }
        total_records += records.size();
        mgr.unpin(f, false, error);
    }
    std::cout << "scanned " << blocks << " block(s), " << total_records << " "
              << Ops::kNoun << '\n';
}

// ---------------------------------------------------------------------------
// REPL plumbing
// ---------------------------------------------------------------------------

void print_help() {
    std::cerr << "Commands:\n"
              << "  append <table> <csv-file> <binary-file>\n"
              << "  read <table> <binary-file> <block-number>\n"
              << "  seek <table> <binary-file> <block-number>...\n"
              << "  scan <table> <binary-file>\n"
              << "  bulk <table> <count> <binary-file>\n"
              << "  quit\n"
              << "Tables: person, part\n";
}

void usage_hint(const std::string& command) {
    static const std::unordered_map<std::string, std::string> kUsage = {
        {"append", "append <table> <csv-file> <binary-file>"},
        {"read", "read <table> <binary-file> <block-number>"},
        {"seek", "seek <table> <binary-file> <block-number>..."},
        {"scan", "scan <table> <binary-file>"},
        {"bulk", "bulk <table> <count> <binary-file>"},
    };
    const auto found = kUsage.find(command);
    if (found != kUsage.end()) {
        std::cerr << "usage: " << found->second << '\n';
    } else {
        std::cerr << "unknown command '" << command << "'\n";
        print_help();
    }
}

void unknown_table(const std::string& table) {
    std::cerr << "unknown table '" << table << "' (use person or part)\n";
}

}

int main() {
    // Session configuration comes from conf.json in the working directory:
    // the replacement policy is resolved through the factory and the pool
    // size must be >= 1. A missing or invalid file is fatal — the session
    // would otherwise silently run with different behavior.
    bufman::Config config;
    std::string error;
    if (!bufman::load_config("conf.json", config, std::cerr, error)) {
        std::cerr << error << '\n';
        return 1;
    }
    std::unique_ptr<bufman::ReplacementPolicy> policy =
        bufman::PolicyFactory::instance().create(config.policy, error);
    if (!policy) {
        std::cerr << error << '\n';
        return 1;
    }

    // The manager lives for the whole session: pages cached by one command
    // are hits for the next.
    bufman::BufferManager mgr;
    mgr.init(config.pool_size, std::move(policy));
    FileCache cache;

    const bool interactive = isatty(fileno(stdin)) != 0;
    for (;;) {
        if (interactive) {
            std::cout << "> " << std::flush;
        }
        std::string line;
        if (!std::getline(std::cin, line)) {
            if (interactive) {
                std::cout << '\n';
            }
            break;
        }

        std::istringstream input(line);
        std::vector<std::string> args;
        std::string token;
        while (input >> token) {
            args.push_back(token);
        }
        if (args.empty()) {
            continue;
        }
        const std::string& command = args[0];
        if (command == "quit") {
            break;
        }

        if (command == "append") {
            if (args.size() != 4) {
                usage_hint(command);
            } else if (args[1] == "person") {
                cmd_append<PersonOps>(mgr, cache, args);
            } else if (args[1] == "part") {
                cmd_append<PartOps>(mgr, cache, args);
            } else {
                unknown_table(args[1]);
            }
        } else if (command == "read") {
            if (args.size() != 4) {
                usage_hint(command);
            } else if (args[1] == "person") {
                cmd_read<PersonOps>(mgr, cache, args);
            } else if (args[1] == "part") {
                cmd_read<PartOps>(mgr, cache, args);
            } else {
                unknown_table(args[1]);
            }
        } else if (command == "seek") {
            if (args.size() < 4) {
                usage_hint(command);
            } else if (args[1] == "person") {
                cmd_seek<PersonOps>(mgr, cache, args);
            } else if (args[1] == "part") {
                cmd_seek<PartOps>(mgr, cache, args);
            } else {
                unknown_table(args[1]);
            }
        } else if (command == "scan") {
            if (args.size() != 3) {
                usage_hint(command);
            } else if (args[1] == "person") {
                cmd_scan<PersonOps>(mgr, cache, args);
            } else if (args[1] == "part") {
                cmd_scan<PartOps>(mgr, cache, args);
            } else {
                unknown_table(args[1]);
            }
        } else if (command == "bulk") {
            if (args.size() != 4) {
                usage_hint(command);
            } else if (args[1] == "person") {
                cmd_bulk<PersonOps>(mgr, cache, args);
            } else if (args[1] == "part") {
                cmd_bulk<PartOps>(mgr, cache, args);
            } else {
                unknown_table(args[1]);
            }
        } else {
            usage_hint(command);
        }
    }

    // Flush every dirty page and close every file exactly once, for the
    // whole session.
    const bool flushed = mgr.close_all(error);
    if (!flushed) {
        std::cerr << error << '\n';
        return 1;
    }
    return 0;
}
