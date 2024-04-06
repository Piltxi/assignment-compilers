# LocalOpts: Local Optimization Passes for LLVM's `opt` Tool

First Assignment for the Languages and Compilers course at the University of Modena and Reggio Emilia. The project implements three local optimization passes for LLVM's `opt` tool: Algebraic Identity Optimization, Strength Reduction, and Multi-Instruction Optimization.

## Features

- **Algebraic Identity Optimization:** Reduces computation overhead by simplifying expressions using algebraic identities.
  $$
  \begin{align*}
    \text{Simplify: }  x + 0 &= 0 + x \quad \Rightarrow \quad x \\
    \text{Simplify: }  x \cdot 1 &= 1 \times x \quad \Rightarrow \quad x
  \end{align*}
  $$
- **Strength Reduction:** Makes execution cheaper by converting complex operations into simpler, equivalent ones.
  $$
  \begin{align*}
    \text{Optimize: }  15 \cdot x \quad &\Rightarrow \quad (x \ll 4) - x \\
    \text{Optimize: }  x / 8 \quad &\Rightarrow \quad x \gg 3
  \end{align*}
  $$
- **Multi-Instruction Optimization:** Improves efficiency by condensing sequences of instructions into fewer steps.
  $$
  \begin{align*}
    \text{Consolidate: } & a = b + 1 \\ 
    & c = a - 1 \quad &\Rightarrow \quad a = b + 1, \quad c = b
  \end{align*}
  $$


## Code Structure

`LocalOpts` provides a base class framework for implementing optimization passes. Derived classes like `AlgebraicIdentityPass`, `StrengthReductionPass`, and `MIOptimizationPass` specifically tailor the `runOnBasicBlock` method to apply optimizations to LLVM's basic blocks.

## Installation and Setup

To integrate LocalOpts into your LLVM setup, follow these steps:

1. **File Placement:**
   - Place the implementation `.cpp` files: `AlgebraicIdentityPass.cpp`, `LocalOpts.cpp`, `MIOptimizationPass.cpp`, `StrengthReductionPass.cpp` to `$ROOT/SRC/llvm/lib/Transforms/Utils`.
   - Place `LocalOpts.hpp` in `$ROOT/SRC/llvm/include/llvm/Transforms/Utils`.
   - Place or add the individual entries for LocalOpts in both `PassBuilder.cpp` and `PassRegistry.def` found in `$ROOT/SRC/llvm/lib/Passes/Transforms/Utils`.

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

