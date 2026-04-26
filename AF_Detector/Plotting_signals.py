import pandas as pd
import matplotlib.pyplot as plt
import glob
import sys

stem = sys.argv[1] if len(sys.argv) > 1 else None

stages = [
    ("_1_raw.csv",       "1. Raw signal"),
    ("_2_qrs_peaks.csv", "2. QRS Peaks"),
    ("_3_PWave_BPF.csv", "3. P-Wave Bandpassed"),
]

files = {}
for suffix, title in stages:
    pattern = f"*{suffix}" if stem is None else f"{stem}{suffix}"
    matches = glob.glob(pattern)
    if matches:
        files[suffix] = (matches[0], title)

if not files:
    print("No CSV files found. Run the C++ program first.")
    sys.exit()

# ── Load timestamps, search regions and amplitudes ────────────────────────────
def load_csv(suffix):
    pattern = f"*{suffix}" if stem is None else f"{stem}{suffix}"
    matches = glob.glob(pattern)
    if matches:
        return pd.read_csv(matches[0], header=None, names=["sample", "value"])
    return None

ts_df     = load_csv("_4_QRStimestamps.csv")
region_df = load_csv("_6_SearchRegions.csv")
amp_df    = load_csv("_5_PWaveAmps.csv")

timestamps     = ts_df["value"].values          if ts_df     is not None else []
search_regions = region_df["value"].values      if region_df is not None else []
p_wave_amps    = amp_df["value"].values         if amp_df    is not None else []

# Search regions come in pairs: [start0, end0, start1, end1, ...]
region_starts = search_regions[0::2] if len(search_regions) > 0 else []
region_ends   = search_regions[1::2] if len(search_regions) > 0 else []

fig, axes = plt.subplots(len(files), 1, figsize=(14, 10))
fig.suptitle(f"ECG Processing Stages — {stem or 'first match'}", fontsize=13)

for ax, (suffix, (filepath, title)) in zip(axes, files.items()):
    df = pd.read_csv(filepath, header=None, names=["sample", "value"])
    ax.plot(df["sample"], df["value"], linewidth=0.7)
    ax.set_title(title, fontsize=10)
    ax.set_xlabel("Sample")
    ax.set_ylabel("Amplitude")
    ax.grid(True, alpha=0.3)

    if suffix == "_3_PWave_BPF.csv":
        ymin = df["value"].min()
        ymax = df["value"].max()

        # QRS timestamps — red vertical lines
        if len(timestamps) > 0:
            ax.vlines(timestamps, ymin=ymin, ymax=ymax,
                      color="red", linewidth=0.8, alpha=0.7, label="QRS timestamp")

        # Search regions — blue shaded spans + amplitude line
        for i, (start, end) in enumerate(zip(region_starts, region_ends)):
            # Shaded search region
            ax.axvspan(start, end, alpha=0.15, color="blue")

            # Horizontal amplitude line within the search region
            if i < len(p_wave_amps):
                amp = p_wave_amps[i]
                ax.hlines(amp, xmin=start, xmax=end,
                          color="green", linewidth=1.2, alpha=0.9,
                          label="P-wave amplitude" if i == 0 else "")

        # Add dummy entries for legend
        ax.axvspan(0, 0, alpha=0.15, color="blue", label="P-wave search region")
        ax.legend(fontsize=8, loc="upper right")

plt.tight_layout()
plt.show()