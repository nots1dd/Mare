# Goo

A learning dump + impl of a basic compiler using the LLVM project.

Currently it is a heavily modified version of LLVM's example JIT compiler `Kaleidoscope`.

Documentation regarding what I have understood and modifying the compiler structure coming soon.

## Building

```bash 
cmake -S . -B build 
cmake --build build -j 8
```

There are 2 targets for building:

1. Compiler 
2. Runtime

To build a specific target:

```bash 
cmake --build build --target runtime # similarly for Compiler -> compiler
```

## Running 

> [!IMPORTANT]
> 
> You must have **gcc** or **clang** toolchains for this!
> 

```bash 
make link 
make run 

# Or combine them 
make all
```
