#ifndef EX09_BUFFER_MANAGER_H
#define EX09_BUFFER_MANAGER_H

#include "file/BlockFile.h"
#include "bufpool/BufferPool.h"
#include "policy/ReplacementPolicy.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace bufman {

// Sits between queries and disk: owns a BufferPool and moves 4096-byte
// blocks between files and frames through BlockFile. Tracks which
// (file, block) lives in which frame, evicts via an injected
// ReplacementPolicy, and writes dirty victims back before reuse.
class BufferManager {
public:
    // Closes every open file (flushing dirty pages first). Runs from the
    // destructor too, so an omitted close_all cannot leak descriptors.
    ~BufferManager();

    void init(std::size_t pool_size, std::unique_ptr<ReplacementPolicy> policy);
    // The manager owns every File it opens; `file` is a non-owning pointer
    // that stays valid until close_all/destruction. There is exactly one
    // File object per descriptor, so a stale handle cannot hold a recycled
    // fd value.
    bool open_file(const std::string& path, bufman::File*& file, std::string& error);
    bool open_file_read(const std::string& path, bufman::File*& file, std::string& error);
    bool pin(bufman::File& file, std::uint64_t block_number,
             std::size_t& frame, std::string& error);
    // Creates a brand-new page: takes a frame, zeroes its buffer, registers
    // it as (file, block_number) and marks it dirty so it reaches disk on
    // write-back. Refuses blocks that are resident, already on disk, or not
    // exactly the next block after the end of file (allocating further out
    // would leave zero-filled gaps that break the whole-blocks invariant).
    bool alloc_page(bufman::File& file, std::uint64_t block_number,
                    std::size_t& frame, std::string& error);
    // True if the page is currently resident in the pool (a pin would hit).
    bool resident(const bufman::File& file, std::uint64_t block_number) const;

    void unpin(std::size_t frame, bool was_dirty, std::string& error);
    DataFrame& frame(std::size_t index);
    const DataFrame& frame(std::size_t index) const;
    bool flush(std::size_t frame, std::string& error);
    bool flush_all(std::string& error);
    // Flushes every dirty page, closes all handles and clears all state.
    // Teardown continues even if a flush fails; the return value reports
    // whether the flush succeeded.
    bool close_all(std::string& error);

private:
    struct PageKey {
        int fd;
        std::uint64_t block_number;
        bool operator==(const PageKey&) const = default;
    };
    struct PageKeyHash {
        std::size_t operator()(const PageKey& key) const;
    };

    // Picks a free frame, or asks the policy to pick among the unpinned
    // page-holding frames. The policy's answer is validated before use; an
    // invalid or absent answer fails with an error.
    bool find_victim_frame(std::size_t& frame, std::string& error) const;
    bool evict_frame(std::size_t frame, std::string& error);
    bufman::File* find_file(int fd);

    BufferPool pool_;
    std::unique_ptr<ReplacementPolicy> policy_;
    std::unordered_map<PageKey, std::size_t, PageKeyHash> page_table_;
    std::vector<std::optional<PageKey>> frame_page_;
    std::deque<bufman::File> files_;   // stable addresses for returned pointers
};

}

#endif // EX09_BUFFER_MANAGER_H
