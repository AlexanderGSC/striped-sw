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
## Performance Analysis: Query Scaling (Fixed Database = 1,000,000 bp)

Evaluated natively on real silicon (**Banana Pi BPI-F3**, SpaceMit K1 octacore, RISC-V RVV 1.0 at `LMUL = 1`) comparing the scalar baseline against the vectorized Farrar Striped implementation:

| Query ($M$) | Database ($N$) | Matrix Cells ($M \times N$) | Scalar Time (MCUPS) | RVV 1.0 Time (MCUPS) | Speedup | Architectural Analysis |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **100** | **1,000,000** | $10^8$ | $1.50 \text{ s}$ ($66.6$) | **$0.47 \text{ s}$ ($212.5$)** | **$3.19\times$** | Setup and vector loop initialization overhead dominates. |
| **200** | **1,000,000** | $2 \times 10^8$ | $3.00 \text{ s}$ ($66.7$) | **$0.73 \text{ s}$ ($273.9$)** | **$4.11\times$** | Vector pipeline warm-up and increased instruction density. |
| **500** | **1,000,000** | $5 \times 10^8$ | $7.47 \text{ s}$ ($66.9$) | **$1.60 \text{ s}$ ($311.8$)** | **$4.66\times$** | Near-optimal vector register utilization. |
| **1,000** | **1,000,000** | $10^9$ | $14.92 \text{ s}$ ($67.0$) | **$3.03 \text{ s}$ ($\mathbf{330.3}$)** | **$\mathbf{4.93\times}$** | **Peak Throughput (L1 Data Cache Sweet Spot).** |
| **2,000** | **1,000,000** | $2 \times 10^9$ | $29.80 \text{ s}$ ($67.1$) | **$7.19 \text{ s}$ ($278.3$)** | **$4.15\times$** | Cache line eviction / L1 miss pressure degradation. |

> 💡 **Key Takeaways:**
> * **Scalar Baseline Consistency:** The classic scalar implementation exhibits near-constant execution throughput ($\approx 66.7 \text{ MCUPS}$), confirming a stable hardware testbench and clock frequency.
> * **Optimal Working Set ($Q = 1000$):** Peak efficiency reaches **$330.3 \text{ MCUPS}$** ($4.93\times$ speedup). At this query length, vector striping aligns perfectly with the L1 Data Cache capacity and register file layout.
> * **Memory Hierarchy Bottleneck ($Q = 2000$):** Performance drops to $278.3 \text{ MCUPS}$ as intermediate vector buffer arrays outgrow the L1 cache footprint, increasing strided memory access stalls.


## Performance Analysis: Problem Size Scaling ($M = N$ Square Matrix)

Evaluated natively on real silicon (**Banana Pi BPI-F3**, SpaceMit K1 octacore, RISC-V RVV 1.0 at `LMUL = 1`) evaluating the asymptotic algorithmic scaling ($O(N^2)$) from small sequences up to $50,000 \times 50,000$ alignment matrices:

| Matrix Size ($M \times N$) | Total Cells | Scalar Time (MCUPS) | RVV 1.0 Time (MCUPS) | Speedup | Architectural Behavior |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **$100 \times 100$** | $10^4$ | $0.16 \text{ ms}$ ($61.5$) | **$0.09 \text{ ms}$ ($104.8$)** | **$1.70\times$** | Setup and loop overhead dominant at ultra-short lengths. |
| **$200 \times 200$** | $4 \times 10^4$ | $0.61 \text{ ms}$ ($65.5$) | **$0.22 \text{ ms}$ ($180.3$)** | **$2.75\times$** | Rapid vector pipeline warm-up. |
| **$500 \times 500$** | $2.5 \times 10^5$ | $3.79 \text{ ms}$ ($65.9$) | **$1.00 \text{ ms}$ ($249.9$)** | **$3.79\times$** | Approaching high vector register occupancy. |
| **$1,000 \times 1,000$** | $10^6$ | $15.20 \text{ ms}$ ($65.8$) | **$3.26 \text{ ms}$ ($306.8$)** | **$4.66\times$** | **Optimal L1 cache locality plateau.** |
| **$5,000 \times 5,000$** | $2.5 \times 10^7$ | $383.30 \text{ ms}$ ($65.2$) | **$84.05 \text{ ms}$ ($297.4$)** | **$4.56\times$** | Sustained high throughput ($\approx 300 \text{ MCUPS}$). |
| **$10,000 \times 10,000$** | $10^8$ | $1.51 \text{ s}$ ($66.2$) | **$0.32 \text{ s}$ ($\mathbf{310.5}$)** | **$\mathbf{4.69\times}$** | **Peak Workload Throughput.** |
| **$50,000 \times 50,000$** | $2.5 \times 10^9$ | $37.71 \text{ s}$ ($66.3$) | **$11.74 \text{ s}$ ($213.0$)** | **$3.21\times$** | Heavy L1/L2 cache thrashing & RAM memory bus starvation. |

>  **Architectural Summary:**
> * **Asymptotic Peak:** The RVV 1.0 vector unit reaches a stable efficiency plateau of **$\sim 300 - 310 \text{ MCUPS}$** for workloads ranging between $10^6$ and $10^8$ total matrix cells.
> * **L2/DRAM Bottleneck Boundary:** Beyond $10.000 \times 10.000$ elements, intermediate vector state buffers exceed the physical hardware cache boundaries, resulting in a **$\sim 31\%$ throughput degradation** due to DRAM latency penalties during vector register reloads.
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
