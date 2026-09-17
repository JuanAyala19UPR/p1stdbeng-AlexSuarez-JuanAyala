# p1stdbeng

A small database engine built piece by piece: fixed-size records (persons,
parts) packed into 4096-byte blocks, a block file layer, and a buffer manager
with a pluggable replacement policy (LRU included).

## Project layout

```
person/   Person record, serializer (21-byte layout), CSV loader, generator
part/     Part record, serializer (36-byte layout), CSV loader, generator
file/     BlockFile — raw POSIX block I/O (4096-byte blocks)
bufpool/  DataFrame, BufferPool, BufferManager (page table + eviction)
policy/   ReplacementPolicy interface + LRUPolicy
```

Two executables are built from shared sources compiled into the `dbcore`
static library:

| Target         | Source     | What it is |
|----------------|------------|------------|
| `p1stdbeng`      | `main.cpp` | PASS/FAIL check suite for the whole engine |
| `p1stdbeng_demo` | `p1db.cpp` | CLI for appending and reading record files |

## Missing implementation files — read before building

This repository does not build yet, and that is deliberate: the seven
implementation files below are not provided — writing them is the
assignment.

```
part/PartSerializer.cpp
part/PartCsv.cpp
part/PartGenerator.cpp
policy/FIFOPolicy.cpp
policy/LFUPolicy.cpp
policy/LRUv2Policy.cpp
policy/MRUPolicy.cpp
```

Until each of these files exists, CMake stops at configure time with an
error like:

```
CMake Error at CMakeLists.txt:6 (add_library):
  Cannot find source file:

    part/PartSerializer.cpp
```

Create the files at exactly those paths — `CMakeLists.txt` already lists
them, and neither it nor any header should be modified. A good first step
is an empty stub for every function the matching header declares: each
serializer/CSV/generator function returning `false`, `0` or `{}`, and each
policy with empty hooks and a `pick_victim` that returns a valid but
arbitrary pick. For example:

```cpp
// policy/FIFOPolicy.cpp — stub; replace with your implementation
#include "FIFOPolicy.h"

namespace bufman {

void FIFOPolicy::init(std::size_t) {}
void FIFOPolicy::on_access(std::size_t) {}
void FIFOPolicy::on_load(std::size_t) {}
void FIFOPolicy::on_remove(std::size_t) {}
std::optional<std::size_t> FIFOPolicy::pick_victim(
        const std::vector<std::size_t>& candidates) const {
    // Valid but arbitrary: keeps the harness running. A stub that returns
    // std::nullopt makes the check suite abort partway.
    if (candidates.empty()) {
        return std::nullopt;
    }
    return candidates.front();
}

}
```

With all seven stubs in place the whole project compiles and the check
suite runs to completion. It will not pass: the checks exercising the
provided code pass, and the ones exercising your seven modules FAIL —
those failures are your to-do list. (A policy stub whose `pick_victim`
returns `std::nullopt` instead of a valid pick makes the run abort
partway; the recommended stub above avoids that.) Work one module at
a time: implement, rebuild, rerun `build/p1stdbeng`, and watch the
failures disappear. The headers are the specification (they also document
the expected design for each module), `EXPLAIN.md` describes each
module's contract, and `build/p1stdbeng` is the acceptance test.

## Building

Requires CMake 4.3+ and a C++20 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The binaries end up in `build/p1stdbeng` and `build/p1stdbeng_demo`.

## Running the check suite

```sh
build/p1stdbeng
```

Takes no arguments. It prints one `PASS:`/`FAIL:` line per check (covering
the buffer manager core, the replacement-policy contract, person and part
record access through frames, the CSV loaders, and `alloc_page`), then a
summary. Exit code is `0` when all checks pass, `1` otherwise.

```sh
build/p1stdbeng | tail -1        # e.g. "All checks passed."
```

Or through CTest:

```sh
cd build && ctest
```

## Running the demo CLI

```sh
build/p1stdbeng_demo
```

Takes no command-line parameters. It starts an interactive session: one
buffer manager (and its pool) lives for the whole session, and files are
opened on first use and kept open. Type commands at the `> ` prompt; `quit`
(or end-of-input) flushes everything, closes the files and exits.

When input is piped in, the prompt is suppressed, so sessions can be
scripted: `printf 'scan person people.bin\nquit\n' | build/p1stdbeng_demo`.

### Commands

`<table>` selects the record type: `person` or `part`. A binary file written
by one record type must only be read by that type's commands.

| Command | Parameters |
|---------|------------|
| `append` | `append <table> <csv-file> <binary-file>` |
| `read`   | `read <table> <binary-file> <block-number>` |
| `seek`   | `seek <table> <binary-file> <block-number>...` (one or more) |
| `scan`   | `scan <table> <binary-file>` |
| `bulk`   | `bulk <table> <count> <binary-file>` |
| `quit`   | flushes and exits |

### Example session

```sh
$ build/p1stdbeng_demo
> append person people.csv people.bin
appended 2 record(s), skipped 1 row(s)
> bulk person 400 people.bin
appended 400 generated record(s)
> scan person people.bin
block 0: 195 record(s)
...
> seek person people.bin 0
block 0 (pool hit): 195 record(s)
> quit
```

Because the pool spans commands, pages read by `scan` are hits for a later
`read`/`seek` of the same block. Errors (unknown command or table, bad block
number, missing file) print a message and the session continues; a file
first used by a read-only command refuses later `append`/`bulk` until you
`quit` and restart. Exit code is `1` only if the final flush fails.

### CSV input format

Person files — each row must have exactly four comma-separated fields. A
header row is accepted but counts as one skipped row (`line 1: pid must be a
positive integer`):

```
pid,name,age,city
1,Alice,30,PR
2,Bob,25,NY
```

Validation rules: `pid > 0`, `age >= 0`, name up to 9 characters, city up to
2 characters.

Part files — each row must have exactly six comma-separated fields:

```
part_id,part_name,part_weight,part_color,part_price,part_material
1,Bolt,0.25,0,1.99,steel
2,Nut,0.1,1,0.75,brass
```

Validation rules: `part_id > 0`, `part_weight >= 0`, `part_color` an integer
in [0, 5], `part_price >= 0`, name and material up to 9 characters. Weight
and price are parsed strictly as numbers (fully consumed, no whitespace, no
overflow).

For both loaders: invalid rows are reported as `line N: reason` on stderr
and skipped (never fatal); the session simply continues.

## Configuration

Both programs read `conf.json` from the working directory at startup:

```json
{
  "policy": "lru",
  "pool_size": 10
}
```

- `policy` — name of the replacement policy, resolved through a factory;
  `fifo`, `lfu`, `lru`, `lruv2` and `mru` are the shipped policies.
  `lru`/`lruv2` evict the least recently accessed frame (vector-indexed vs
  the textbook list + hash-map idiom); `mru` evicts the most recently
  accessed one, which keeps cyclic scans over files larger than the pool
  from thrashing; `lfu` evicts the least frequently accessed one (count
  ties broken by recency); `fifo` evicts the earliest arrival regardless
  of hits. New policies register themselves with
  `PolicyFactory::register_policy` and become usable from the file.
- `pool_size` — number of frames in the buffer pool, an integer >= 1.

A missing, malformed, or invalid `conf.json` is fatal for the session:
unknown policy, `pool_size` < 1, duplicate keys, trailing commas or garbage
are all reported with a reason. Unknown keys are skipped with a warning.
The check suite also honors `conf.json` — for its session-shaped scan demo —
and defaults to `lru`/8 when the file is absent; its behavior tests keep
explicit pool sizes so eviction-order assertions stay deterministic.

## On-disk format

A file is always a whole number of 4096-byte blocks. New pages are created
through the buffer manager's `alloc_page` (allocated densely, no gaps), and
appends fill free slots in the last block before allocating new blocks.

Person files: up to 195 records of 21 bytes each per block (`pid` 4 bytes,
`name` 10, `age` 4, `city` 3, packed back to back). Part files: up to 113
records of 36 bytes each per block (`part_id` 4, `part_name` 10,
`part_weight` 4, `part_color` 4, `part_price` 4, `part_material` 10). In
both layouts a zeroed slot (`pid == 0` / `part_id == 0`) means "free", and
bytes are written in native byte order (32-bit `int`/`float` assumed).

## Documentation

`EXPLAIN.md` explains every module, the design decisions behind them, and
how the layers cooperate. Read it top to bottom.
