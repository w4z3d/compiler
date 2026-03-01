# c0 Compiler

A C0 compiler built in C++ with no external dependencies.

C0 is a safe subset of C designed for teaching. It features garbage-collected heap allocation, bounds-checked arrays, and contracts, while retaining C like expression syntax and control flow.

## Features

- Recursive descent parser
- SSA-based intermediate representation constructed directly from the AST using the algorithms described in [Braun et al., "Simple and Efficient Construction of Static Single Assignment Form" (CC 2013)](https://link.springer.com/chapter/10.1007/978-3-642-37051-9_6)
- Pruned and minimal SSA form for reducible control flow
- Register allocation via greedy coloring on chordal interference graphs
- Zero external dependencies
- Arena allocator

## Building

Clang is strongly recommended. The codebase uses C++23 features that may not be fully supported by other compilers.

```bash
# Clone
git clone https://github.com/w4z3d/compiler.git
cd compiler

# Build with CMake and Ninja
mkdir build && cd build
cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -G Ninja ..
cmake --build .

```

## Usage

```bash
./compiler examples/test.c0 <output>
```

## References

- M. Braun, S. Buchwald, S. Hack, R. Leißa, C. Mallon, A. Zwinkau. _Simple and Efficient Construction of Static Single Assignment Form_. CC 2013.
- S. Hack, D. Grund, G. Goos. _Register Allocation for Programs in SSA Form_. CC 2006.
