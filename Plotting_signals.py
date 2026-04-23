import pandas as pd
import matplotlib.pyplot as plt
import glob
import sys
import numpy as np
import os
# Set this to your project folder where C++ writes the CSVs
CSV_DIR = r"C:\Users\timmy\OneDrive\TCD Engineering\SS - Semester 2\Computers in Medicine\Project - Single Lead AF Classification\BME-463"

stem = sys.argv[1] if len(sys.argv) > 1 else ""

def load_csv(suffix):
    filepath = os.path.join(CSV_DIR, suffix)
    if os.path.exists(filepath):
        df = pd.read_csv(filepath, header=None, names=["sample", "value"])
        print(f"Loaded {suffix}: {len(df)} rows")
        return df
    print(f"Not found: {filepath}")
    return None

# ── Load all signals ──────────────────────────────────────────────────────────
raw_df      = load_csv("_1_raw.csv")
qrs_df      = load_csv("_2_qrs_peaks.csv")
pwave_df    = load_csv("_3_PWave_BPF.csv")
ts_df       = load_csv("_4_QRStimestamps.csv")
amp_df      = load_csv("_5_PWaveAmps.csv")
region_df   = load_csv("_6_SearchRegions.csv")
template_df = load_csv("_7_template.csv")
diff_df     = load_csv("_8_diff_signal.csv")  

timestamps     = ts_df["value"].values     if ts_df     is not None else []
search_regions = region_df["value"].values if region_df is not None else []
p_wave_amps    = amp_df["value"].values    if amp_df    is not None else []
region_starts  = search_regions[0::2]      if len(search_regions) > 0 else []
region_ends    = search_regions[1::2]      if len(search_regions) > 0 else []

if raw_df is None:
    print("No CSV files found. Run the C++ program first.")
    sys.exit()

# ── Figure 1: Main processing stages + diff ───────────────────────────────────
subplots = [
    (raw_df,   "1. Raw signal",          "blue"),
    (qrs_df,   "2. QRS Peaks",           "green"),
    (pwave_df, "3. P-Wave Bandpassed",   "orange"),
    (diff_df,  "4. Template difference", "coral"),
]
active = [(df, title, color) for df, title, color in subplots if df is not None]

for df, title, color in subplots:
    print(f"{title}: {'loaded' if df is not None else 'NONE'}")

active = [(df, title, color) for df, title, color in subplots if df is not None]
print(f"Active subplots: {len(active)}")

fig, axes = plt.subplots(len(active), 1, figsize=(14, 3 * len(active)))
fig.suptitle(f"ECG Processing Stages - Raw Signal", fontsize=13)

if len(active) == 1:
    axes = [axes]

for ax, (df, title, color) in zip(axes, active):
    ax.plot(df["sample"], df["value"], linewidth=0.7, color=color)
    ax.set_title(title, fontsize=10)
    ax.set_ylabel("Amplitude")
    ax.grid(True, alpha=0.3)

    # Overlay QRS timestamps on P-wave and diff subplots
    if title in ("3. P-Wave Bandpassed", "4. Template difference") \
            and len(timestamps) > 0:
        ax.vlines(timestamps,
                  ymin=df["value"].min(), ymax=df["value"].max(),
                  color="red", linewidth=0.8, alpha=0.7,
                  label="QRS timestamp")

    # P-wave search regions and amplitude lines on P-wave subplot only
    if title == "3. P-Wave Bandpassed":
        for i, (start, end) in enumerate(zip(region_starts, region_ends)):
            ax.axvspan(start, end, alpha=0.15, color="blue")
            if i < len(p_wave_amps):
                ax.hlines(p_wave_amps[i], xmin=start, xmax=end,
                          color="green", linewidth=1.2, alpha=0.9,
                          label="P-wave amplitude" if i == 0 else "")
        ax.axvspan(0, 0, alpha=0.15, color="blue", label="P-wave search region")

    # Zero line on diff subplot
    if title == "4. Template difference":
        ax.axhline(y=0, color="black", linewidth=0.5, linestyle="--")

    # Only add legend if this subplot has labelled artists
    handles, labels = ax.get_legend_handles_labels()
    if labels:
        ax.legend(fontsize=8, loc="upper right")

axes[-1].set_xlabel("Sample")
plt.tight_layout()
plt.show()

# ── Figure 2: Template ────────────────────────────────────────────────────────
if template_df is not None:
    fig2, ax2 = plt.subplots(figsize=(10, 4))
    fig2.suptitle("Beat template debugging", fontsize=13)

    ax2.plot(template_df["sample"], template_df["value"],
             linewidth=1.2, color="purple", label="Template")
    ax2.axvline(x=60, color="red", linewidth=0.8,
                linestyle="--", label="QRS position (halfWin=60)")
    ax2.set_title("Beat template (average of beats 4-6)", fontsize=10)
    ax2.set_xlabel("Sample (relative to window start)")
    ax2.set_ylabel("Amplitude")
    ax2.legend(fontsize=8)
    ax2.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.show()
