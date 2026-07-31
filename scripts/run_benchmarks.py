#!/usr/bin/env python3
import os
import re
import subprocess
import json
import csv
from typing import Dict, Any, List


# ==============================================================================
# CONFIGURATION
# ==============================================================================
# Path to directory containing benchmark binaries
BIN_DIR = "./"

# List of binaries to execute (mapping to different LMUL configurations)
BINARIES = ["bench3m1", "bench3m2", "bench3m4", "bench3m8"]

# Test matrix: Query sizes (|Q|) and Database sizes (|DB|)
QUERY_SIZES = [500,1000,1500,2000,2500,3000,3500,4000,4500,5000,5500,6000,6500,7000,7500,8000,8500,9000,9500,10000]
DATABASE_SIZES = [100000]

# Output files
OUTPUT_CSV = "benchmark_results.csv"
OUTPUT_JSON = "benchmark_results.json"


# ==============================================================================
# PARSER FUNCTION
# ==============================================================================
def parse_benchmark_output(output_text: str) -> Dict[str, Any]:
    """
    Parses the standard output of the Smith-Waterman benchmark program
    using regular expressions to extract metrics into a dictionary.
    """
    metrics: Dict[str, Any] = {}

    # Regex patterns mapped to metric names
    patterns = {
        "lmul": r"===\s*STRIP SMITH-WATERMAN RISC-V RVV \(LMUL=(\d+)\)\s*===",
        "execution_time_s": r"Execution Time:\s*([\d\.]+)\s*s",
        "throughput_mcups": r"Throughput:\s*([\d\.]+)\s*MCUPS",
        "throughput_gcups": r"Throughput:\s*[\d\.]+\s*MCUPS\s*\(([\d\.]+)\s*GCUPS\)",
        "cells_per_cycle": r"Cells/Cycle:\s*([\d\.]+)",
        "cells_per_inst": r"Cells/Inst:\s*([\d\.]+)",
        "cycles": r"Cycles:\s*(\d+)",
        "instructions": r"Instructions:\s*(\d+)",
        "ipc": r"IPC:\s*([\d\.]+)",
        "l1_loads": r"L1 Loads:\s*(\d+)",
        "l1_misses": r"L1 Misses:\s*(\d+)",
        "l1_miss_rate": r"L1 Miss Rate:\s*([\d\.]+)%",
        "frontend_stalls": r"Frontend Stalls:\s*(\d+)",
        "backend_stalls": r"Backend Stalls:\s*(\d+)",
        "stalls_rate": r"Stalls Rate:\s*([\d\.]+)%",
        "branches": r"Branches:\s*(\d+)",
        "branch_misses": r"Branch Misses:\s*(\d+)",
        "branch_miss_pct": r"Branch Miss %:\s*([\d\.]+)%",
    }

    for key, pattern in patterns.items():
        match = re.search(pattern, output_text)
        if match:
            val_str = match.group(1)
            if "." in val_str:
                metrics[key] = float(val_str)
            else:
                metrics[key] = int(val_str)
        else:
            metrics[key] = None

    return metrics


# ==============================================================================
# MAIN EXECUTION LOOP
# ==============================================================================
def run_experiments() -> List[Dict[str, Any]]:
    results: List[Dict[str, Any]] = []

    total_runs = len(BINARIES) * len(QUERY_SIZES) * len(DATABASE_SIZES)
    current_run = 0

    print(f"Starting benchmark suite. Total runs planned: {total_runs}\n")

    for binary in BINARIES:
        bin_path = os.path.join(BIN_DIR, binary)

        if not os.path.exists(bin_path):
            print(f"[WARNING] Binary '{bin_path}' not found. Skipping...")
            continue

        for db_len in DATABASE_SIZES:
            for q_len in QUERY_SIZES:
                current_run += 1
                cmd = [bin_path, str(db_len), str(q_len)]

                print(f"[{current_run}/{total_runs}] Running: {' '.join(cmd)} ... ", end="", flush=True)

                try:
                    process = subprocess.run(
                        cmd,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        text=True,
                        check=True
                    )
                    
                    run_data = parse_benchmark_output(process.stdout)
                    
                    # Metadata added at the front
                    full_record = {
                        "binary": binary,
                        "lmul": run_data.get("lmul"),
                        "query_length": q_len,
                        "database_length": db_len,
                        "execution_time_s": run_data.get("execution_time_s"),
                        "throughput_mcups": run_data.get("throughput_mcups"),
                        "ipc": run_data.get("ipc"),
                        "l1_miss_rate": run_data.get("l1_miss_rate"),
                        "stalls_rate": run_data.get("stalls_rate")
                    }
                    
                    # Merge remaining parsed hardware counters
                    full_record.update(run_data)
                    results.append(full_record)
                    print("SUCCESS")

                except subprocess.CalledProcessError as e:
                    print(f"FAILED (Exit Code: {e.returncode})")
                    print(f"Error output: {e.stderr.strip()}")
                except Exception as e:
                    print(f"ERROR: {str(e)}")

    return results


# ==============================================================================
# ENTRY POINT & EXPORT (PURE STANDARD LIBRARY)
# ==============================================================================
if __name__ == "__main__":
    raw_results = run_experiments()

    if not raw_results:
        print("No valid results were collected.")
        exit(1)

    # 1. Export to JSON (Native)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(raw_results, f, indent=4)
    print(f"\n[INFO] Raw JSON data saved to: {OUTPUT_JSON}")

    # 2. Export to CSV (Native `csv` module)
    fieldnames = list(raw_results[0].keys())
    with open(OUTPUT_CSV, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(raw_results)
    print(f"[INFO] Processed dataset saved to: {OUTPUT_CSV}")

    # 3. Print Summary Table using String Formatting (Native)
    print("\n" + "=" * 80)
    print("EXECUTION SUMMARY")
    print("=" * 80)
    header_fmt = "{:<10} {:<6} {:<10} {:<18} {:<8} {:<14} {:<12}"
    row_fmt    = "{:<10} {:<6} {:<10} {:<18.2f} {:<8.4f} {:<14.4f} {:<12.4f}"
    
    print(header_fmt.format("Binary", "LMUL", "Query|Q|", "Throughput(MCUPS)", "IPC", "L1 Miss (%)", "Stalls (%)"))
    print("-" * 80)

    for r in raw_results:
        print(row_fmt.format(
            str(r.get("binary", "")),
            str(r.get("lmul", "")),
            str(r.get("query_length", "")),
            r.get("throughput_mcups") or 0.0,
            r.get("ipc") or 0.0,
            r.get("l1_miss_rate") or 0.0,
            r.get("stalls_rate") or 0.0
        ))
    print("=" * 80)