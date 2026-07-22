# Sequence Alignment Using the Vectorised Smith-Waterman Algorithm

[![CI Build & Test](https://github.com/AlexanderGSC/striped-sw/actions/workflows/ci.yml/badge.svg)](https://github.com/AlexanderGSC/striped-sw/actions)
![Compiler GCC](https://img.shields.io/badge/GCC-13.3+-A42E2B?style=flat&logo=gnu)
![Compiler Clang](https://img.shields.io/badge/Clang-18+-074B83?style=flat&logo=llvm)
![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)
![Architecture](https://img.shields.io/badge/Architecture-RISC--V%20%7C%20x86__64-orange.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

>An optimized C++23 implementation of the Striped Smith-Waterman algorithm proposed by Michael Farrar, transitioning from a basic reference to hardware-accelerated SIMD (SSE/AVX) and RISC-V(RVV).


## Problem Description

The **Smith-Waterman algorithm** is the gold standard for local sequence alignment in bioinformatics. However, its classic dynamic programming formulation has a quadratic time complexity of $O(m \times n)$, where $m$ and $n$ are the lengths of the query and database sequences. 

The recurrence relation dictates that each cell in the scoring matrix $H$ depends on its top, left, and diagonal neighbors:

$$H_{i,j} = \max \begin{cases} 0 \\ H_{i-1,j-1} + S(Q_i, D_j) \\ E_{i,j} \quad \text{(horizontal gap)} \\ F_{i,j} \quad \text{(vertical gap)} \end{cases}$$

Because of these tight data dependencies ($H_{i,j}$ depends on $H_{i-1,j}$ and $H_{i,j-1}$), parallelizing this algorithm using SIMD (Single Instruction, Multiple Data) vectors is highly challenging. 

### The Striped Approach
To bypass these dependency bottlenecks, **Michael Farrar (2007)** proposed the **Striped Smith-Waterman** method. Instead of processing the matrix parallel to the query or diagonally, the query sequence is divided into parallel segments (stripes) equal to the SIMD vector width. 

This approach:
* Increases instruction-level parallelism.
* Minimizes expensive vector-shift operations.
* Introduces a **Lazy F-loop** to handle and propagate delayed vertical gap corrections ($F_{i,j}$) across SIMD boundary segments.

---

## Project Status & Architecture

This repository is structured to show a clean evolutionary path from absolute logical correctness to bare-metal hardware optimization:

1. **`Golden Reference Smith Waterman`:** A C++20/23 implementation of classic Smith-Waterman, used as a validation reference in unit tests.

2. **`RISC-V RVV Acceleration`:** An emulated version of the algorithm using ```std::vector``` containers instead of architecture-specific vector registers. This algorithm serves as a validation basis for the final vectorized algorithm, as well as a version for compiler autovectorization. 
   
2. **`RISC-V RVV Acceleration`:**
Translation of the Strip-Smith-Waterman algorithm’s logic to the RISC-V SIMD model using vector extensions. Vector intrinsics for different LMULs are derived through template specialization at compile time. This template specialization is found in the file ```rvv-traits.hpp```. Performance measurements will be conducted on the ***Banana BPI F3*** SBC, which  features eight ***SpaceMIT K1*** cores with a 256-bit VLEN. 

3. **`SIMD Acceleration` (Work in Progress):**
   The next phase involves mapping the validated emulated logic directly to hardware vector intrinsics (SSE4.1, AVX2, and AVX-512) to achieve massive performance speedups.

---

### Getting Started

To compile and run the current emulated golden reference:

```bash
# Compile using C++23
cmake -B build   -DCMAKE_C_COMPILER=riscv64-linux-gnu-gcc   -DCMAKE_CXX_COMPILER=riscv64-linux-gnu-g++   -DCMAKE_BUILD_TYPE=Release

cmake --build build

# Run the alignment tests
build/unit_test1
build/unit_test2
```
An example of alignment

```text
ALIGNMENT
A G C - T G A C - A T C G A T A C G A G C T G G C T A - G A C A T T C A C G A T A C G 
| | |   | |   |   |     |   | | |   |       | | | | |   |     | | |   | |   |   |   | 
A G C T T G - C G A - - G - T A C - A - - - G G C T A - G - - A T T - A C - A - A - G 
```

### References
* **Farrar's Striped Algorithm:** Farrar, M. (2007). *Striped Smith–Waterman database searching instruments*. Bioinformatics, 23(2), 156-161. [DOI: 10.1093/bioinformatics/btl582](https://doi.org/10.1093/bioinformatics/btl582)
* **Original Smith-Waterman:** Smith, T. F., & Waterman, M. S. (1981). *Identification of common molecular subsequences*. Journal of Molecular Biology, 147(1), 195-197. [DOI: 10.1016/0022-2836(81)90087-5](https://doi.org/10.1016/0022-2836(81)90087-5)

