# Cachi: a Multilevel Processor Cache Simulator

Cachi is a cache simulator written in C. It supports configurable cache
hierarchies, replacement policies, and Valgrind Lackey memory traces.

## Build

From the project root, run:

```sh
make
```

This creates the `cachi` executable. To build the debug executable, run
`make debug`; it creates `cachi-debug`.

## Run

Pass a cache hierarchy configuration file and a memory trace:

```sh
./cachi configs/example-multilevel.cfg benchmarks/traces/fib-trace.txt
```

Example configurations are provided in `configs/`. Benchmark programs are in
`benchmarks/programs/`, with ready-to-run traces for array, Fibonacci, and
strided access patterns in `benchmarks/traces/`.

The simulator prints hit, miss, eviction, and writeback counts for each cache
level. `example-single-level.cfg` demonstrates a direct-mapped L1; the
multilevel example demonstrates a three-level hierarchy with several
replacement policies.

## Output

The simulator prints one line per cache level, for example:

```text
L1: accesses=1000 hits=900 misses=100 hit_rate=90.00% miss_rate=10.00% evictions=36 writebacks=12
```

- `accesses` is the number of requests that reached that level (`hits + misses`).
- `hits` and `misses` count the lookup results at that level.
- `hit_rate` and `miss_rate` are percentages of that level's accesses.
- `evictions` counts valid cache lines replaced; `writebacks` counts evicted
  dirty lines.

Lower-level caches see two kinds of requests from the level above: a block
fetch (read) for every miss, and a block write for every dirty writeback.
Their access counts and rates are therefore calculated from different requests
than L1's. Writebacks from the last level go to memory, which is not modeled.

## Write behavior

Every level is write-back and write-allocate. A store marks the line dirty in
the level it hits (or is allocated in) only. When a dirty line is evicted, the
whole block is written to the next level, which may in turn evict and write
back its own dirty lines.

## Configuration format

The configuration file defines one section per cache level. Capacity and block
size are in bytes; associativity is the number of ways per set.

```ini
[L1]
capacity = 32768
block_size = 64
associativity = 8
replacement = LRU
```

Supported replacement names are `LRU`, `FIFO`, `MRU`, and `RR` (random).

## Memory trace format

The trace parser accepts Valgrind Lackey records in the form
`operation address,size`, such as `L 7ff00000,8`. `L` is a read, `S` is a
write, `M` is treated as a read followed by a write, and `I` is ignored. An
access that crosses a block boundary is split into one request per block, using
each level's own block size. The
included benchmark traces contain data operations (`L`, `S`, and `M`); they
were generated from `fib.c` with input `20` and optimized `strided.c` with
stride `64`.

## Clean generated executables

```sh
make clean
```

## Tests

Run the cache, hierarchy, replacement, and trace parser tests with:

```sh
make test
```

## TODO

- Add more complex replacement policies, such as tree pseudo-LRU and RRIP.
- Make the write policy configurable per level (write-through,
  no-write-allocate).
- Add inclusive and exclusive hierarchy modes; lower levels are currently
  non-inclusive and are never back-invalidated.
