import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import io

df = pd.read_csv("benchmark_results.csv")
#df = pd.read_csv(io.StringIO(csv_data))

# Configuración estética profesional
sns.set_theme(style="whitegrid", font_scale=1.0)
palette = {1: "#1f77b4", 2: "#ff7f0e", 4: "#2ca02c", 8: "#d62728"} # Azul, Naranja, Verde (LMUL=4), Rojo (LMUL=8)

fig, axes = plt.subplots(2, 3, figsize=(18, 10))

# -------------------------------------------------------------------------
# Plot 1: Throughput (MCUPS) vs Query Length
# -------------------------------------------------------------------------
sns.lineplot(data=df, x="query_length", y="throughput_mcups", hue="lmul", palette=palette, marker="o", linewidth=2.5, ax=axes[0,0])
axes[0,0].set_title("A. Kernel Throughput (MCUPS)", fontsize=12, fontweight='bold')
axes[0,0].set_ylabel("MCUPS (Higher is better)")
axes[0,0].set_xlabel("Query Length (|Q|)")

# -------------------------------------------------------------------------
# Plot 2: Vector Efficiency (Cells / Instruction)
# -------------------------------------------------------------------------
sns.lineplot(data=df, x="query_length", y="cells_per_inst", hue="lmul", palette=palette, marker="^", linewidth=2.5, ax=axes[0,1])
axes[0,1].set_title("B. Algorithmic Density (Cells / Instruction)", fontsize=12, fontweight='bold')
axes[0,1].set_ylabel("Cells / Instruction")
axes[0,1].set_xlabel("Query Length (|Q|)")

# -------------------------------------------------------------------------
# Plot 3: IPC Degradation
# -------------------------------------------------------------------------
sns.lineplot(data=df, x="query_length", y="ipc", hue="lmul", palette=palette, marker="s", linewidth=2.5, ax=axes[0,2])
axes[0,2].set_title("C. Pipeline IPC (Instructions / Cycle)", fontsize=12, fontweight='bold')
axes[0,2].set_ylabel("IPC")
axes[0,2].set_xlabel("Query Length (|Q|)")

# -------------------------------------------------------------------------
# Plot 4: Backend Stalls %
# -------------------------------------------------------------------------
sns.lineplot(data=df, x="query_length", y="stalls_rate", hue="lmul", palette=palette, marker="d", linewidth=2.5, ax=axes[1,0])
axes[1,0].set_title("D. Pipeline Bottleneck (Backend Stalls %)", fontsize=12, fontweight='bold')
axes[1,0].set_ylabel("Backend Stalls (%)")
axes[1,0].set_xlabel("Query Length (|Q|)")

# -------------------------------------------------------------------------
# Plot 5: L1 Data Cache Miss Rate (%)
# -------------------------------------------------------------------------
sns.lineplot(data=df, x="query_length", y="l1_miss_rate", hue="lmul", palette=palette, marker="X", linewidth=2.5, ax=axes[1,1])
axes[1,1].set_title("E. L1 Data Cache Miss Rate (%)", fontsize=12, fontweight='bold')
axes[1,1].set_ylabel("L1 Miss Rate (%)")
axes[1,1].set_xlabel("Query Length (|Q|)")

# -------------------------------------------------------------------------
# Plot 6: L1 Loads Surge (Demuestra Register Spilling en LMUL=8)
# -------------------------------------------------------------------------
# Convertimos L1 Loads a Millones para fácil lectura
df["l1_loads_millions"] = df["l1_loads"] / 1e6
sns.lineplot(data=df, x="query_length", y="l1_loads_millions", hue="lmul", palette=palette, marker="P", linewidth=2.5, ax=axes[1,2])
axes[1,2].set_title("F. Memory Pressure: L1 Loads (Millions)", fontsize=12, fontweight='bold')
axes[1,2].set_ylabel("L1 Data Loads (Millions)")
axes[1,2].set_xlabel("Query Length (|Q|)")

plt.suptitle("Microarchitectural Characterization of Striped Smith-Waterman on RISC-V RVV (SpaceMIT K1)", fontsize=16, fontweight='bold', y=0.98)
plt.tight_layout(rect=[0, 0, 1, 0.96])

# Guardar en alta calidad para LaTeX / README
plt.savefig("rvv_ssw_profiling_full.svg", format="svg", dpi=300)
plt.savefig("rvv_ssw_profiling_full.png", format="png", dpi=300)
print("Success! Gráphs exported as 'rvv_ssw_profiling_full.png' y '.svg'")