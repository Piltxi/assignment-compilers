# LocalOpts: Local Optimization Passes for LLVM's `opt` Tool

First Assignment for the Languages and Compilers course at the University of Modena and Reggio Emilia. The project implements three local optimization passes for LLVM's `opt` tool: Algebraic Identity Optimization, Strength Reduction, and Multi-Instruction Optimization.

## Features

- **Algebraic Identity Optimization:** Reduces computation overhead by simplifying expressions using algebraic identities.
  - Simplify: `x + 0 = 0 + x => x`
  - Simplify: `x * 1 = 1 * x => x`

- **Strength Reduction:** Makes execution cheaper by converting complex operations into simpler, equivalent ones.
  - Optimize: `15 * x => (x << 4) - x`
  - Optimize: `x / 8 => x >> 3`

- **Multi-Instruction Optimization:** Improves efficiency by condensing sequences of instructions into fewer steps.
  - Consolidate: `a = b + 1, c = a - 1 => a = b + 1, c = b`

## Code Structure

`LocalOpts` provides a base class framework for implementing optimization passes. Derived classes like `AlgebraicIdentityPass`, `StrengthReductionPass`, and `MIOptimizationPass` specifically tailor the `runOnBasicBlock` method to apply optimizations to LLVM's basic blocks.

## Installation and Setup

To integrate LocalOpts into your LLVM setup, follow these steps:

1. **File Placement:**
   - Place the implementation `.cpp` files and the `CMakeLists.txt` file that can be found in the [lib](lib) directory: `AlgebraicIdentityPass.cpp`, `LocalOpts.cpp`, `MIOptimizationPass.cpp`, `StrengthReductionPass.cpp` in `$ROOT/SRC/llvm/lib/Transforms/Utils`.
   - Place `LocalOpts.hpp`, found in the [include](include) directory, in `$ROOT/SRC/llvm/include/llvm/Transforms/Utils`.
   - Place from the [Passes](Passes) directory or add the individual entries for LocalOpts in both `PassBuilder.cpp` and `PassRegistry.def` that can be found in the directory `$ROOT/SRC/llvm/lib/Passes`.

2. **Compilation:**
   - Navigate to your LLVM build directory (`$ROOT/BUILD`).
   - Use `make -j[N] opt` to compile the `opt` tool with the new LocalOpts passes included. Here, `[N]` specifies the number of cores to use for parallel compilation.
   - (Optional) To install the compiled `opt` tool into the LLVM installation directory, run `make install`.

## Usage

To apply the LocalOpts passes to your LLVM IR code, use the following command:

```bash
opt -passes="algebraic-identity,strength-reduction,mi-opt" -S <file_to_optimize>.ll -o <optimized_file>.ll
```

Replace `<file_to_optimize>.ll` with the path to your LLVM IR code file, and `<optimized_file>.ll` with the desired output file path.

## Group Members
| Name  | Matricola |
|-------|-----------|
|Alessandro Catenacci | 164914 |
|Elia Pitzalis | 152541 |
|Francesco Caligiuri | 146666 |
