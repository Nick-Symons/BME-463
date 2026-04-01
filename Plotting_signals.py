import pandas as pd
import matplotlib.pyplot as plt
import glob
import sys

# Pass a file stem as argument, e.g: python plot_stages.py "sample01"
# Or leave blank to plot the first set found
stem = sys.argv[1] if len(sys.argv) > 1 else None

stages = [
    ("_1_raw.csv",        "1. Raw signal"),
    ("_2_resampled.csv",  "2. Resampled (200 Hz)"),
    ("_3_bandpassed.csv", "3. Bandpassed (5-15 Hz)"),
    ("_4_qrs_peaks.csv",  "4. QRS peak output"),
]

# Find files matching the stem or just grab the first match
files = {}
for suffix, title in stages:
    pattern = f"*{suffix}" if stem is None else f"{stem}{suffix}"
    matches = glob.glob(pattern)
    if matches:
        files[suffix] = (matches[0], title)

if not files:
    print("No CSV files found. Run the C++ program first.")
    sys.exit()

fig, axes = plt.subplots(len(files), 1, figsize=(14, 10))
fig.suptitle(f"ECG Processing Stages — {stem or 'first match'}", fontsize=13)

for ax, (suffix, (filepath, title)) in zip(axes, files.items()):
    df = pd.read_csv(filepath, header=None, names=["sample", "value"])
    ax.plot(df["sample"], df["value"], linewidth=0.7)
    ax.set_title(title, fontsize=10)
    ax.set_xlabel("Sample")
    ax.set_ylabel("Amplitude")
    ax.grid(True, alpha=0.3)

plt.tight_layout()
plt.show()