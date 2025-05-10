# Compiler

## Content
<!--toc:start-->
- [How to build](#how-to-build)
<!--toc:end-->
## How to build

- [Linux](#linux)
- [Windows](#windows)

### Linux

```bash
cd build && cmake .. -DCAMKE_BUILD_TYPE=Release && cmake --build . --parallel 8
```

### Windows

```bash
cd build && cmake .. -DCAMKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCAMKE_C_COMPILER=/usr/bin/clang && cmake --build . --parallel 8
```
