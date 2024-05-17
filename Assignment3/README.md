# Loop Invariant Code Motion Pass for LLVM's `opt` Tool

Third Assignment for the Languages and Compilers course at the University of Modena and Reggio Emilia. The project implements a loop optimization pass for LLVM's `opt` tool: Loop Invariant Code Motion Pass.

## Features

- **Loop Invariant Code Motion Pass:** Improves efficiency by moving computations that do not change within a loop outside of the loop.
  - Move: `if (i < n) { x = 10; ... } => x = 10; if (i < n) { ... }`

## Code Structure

The `LoopInvariantHoistPass` class specifically tailors the `runOnLoop` method to apply optimizations to LLVM's loops by identifying and moving loop-invariant code.

## Installation and Setup

To integrate the Loop Invariant Code Motion Pass into your LLVM setup, follow these steps:

1. **File Placement:**
   - Place the implementation `.cpp` file found in the [lib](lib) directory: `LoopInvariantHoistPass.cpp` in `$ROOT/SRC/llvm/lib/Transforms/Utils`.
   - Place `LoopInvariantHoistPass.hpp`, found in the [include](include) directory, in `$ROOT/SRC/llvm/include/llvm/Transforms/Utils`.
   - (Optional) Place the `CMakeLists.txt` file in the `$ROOT/SRC/llvm/lib/Transforms/Utils` directory. This file is included more as a reference and may contain other passes that the user who cloned this may not have.
   - (Optional) Add the individual entry for the pass in `PassBuilder.cpp` and `PassRegistry.def` files in the `$ROOT/SRC/llvm/lib/Passes` directory. These files are also included more as a reference due to the potential presence of other custom passes that the user may not have.

2. **Compilation:**
   - Navigate to your LLVM build directory (`$ROOT/BUILD`).
   - Use `make -j[N] opt` to compile the `opt` tool with the new Loop Invariant Code Motion pass included. Here, `[N]` specifies the number of cores to use for parallel compilation.
   - (Optional) To install the compiled `opt` tool into the LLVM installation directory, run `make install`.

## Usage

To apply the Loop Invariant Code Motion Pass to your LLVM IR code, use the following command:

```bash
opt -passes="custom-licm" -S <file_to_optimize>.ll -o <optimized_file>.ll
```

Replace `<file_to_optimize>.ll` with the path to your LLVM IR code file, and `<optimized_file>.ll` with the desired output file path.

## Group Members
| Name  | Matricola |
|-------|-----------|
|Alessandro Catenacci | 164914 |
|Elia Pitzalis | 152541 |
|Francesco Caligiuri | 146666 |