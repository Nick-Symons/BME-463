import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.metrics import f1_score, confusion_matrix, ConfusionMatrixDisplay

# ── Load and diagnose ─────────────────────────────────────────────────────────
# Try tab first, fall back to comma
try:
    df = pd.read_csv("results.csv", sep="\t")
    if "true_label" not in df.columns:
        df = pd.read_csv("results.csv", sep=",")
except:
    df = pd.read_csv("results.csv", sep=",")

print("Columns found    :", df.columns.tolist())
print("Shape            :", df.shape)
print("Classes found    :", df["true_label"].unique())
print(df.head())

df = df.dropna(subset=["mean_entropy", "true_label"])
df["true_label"] = df["true_label"].str.lower().str.strip()

print(f"\nRecords per class:\n{df['true_label'].value_counts()}")

# ── 1. Box plot — much better than histogram for small datasets ───────────────
fig, ax = plt.subplots(figsize=(10, 5))

classes   = ["af", "normal", "noisy", "other"]
colors    = ["#E24B4A", "#1D9E75", "#EF9F27", "#888780"]
plot_data = [df[df["true_label"] == c]["mean_entropy"].values for c in classes]

bp = ax.boxplot(plot_data, tick_labels=classes, patch_artist=True, notch=False)
for patch, color in zip(bp["boxes"], colors):
    patch.set_facecolor(color)
    patch.set_alpha(0.6)

# Overlay individual points
legend_handles = []
for i, (data, color, label) in enumerate(zip(plot_data, colors, classes)):
    x = np.random.normal(i + 1, 0.06, size=len(data))
    scatter = ax.scatter(x, data, alpha=0.9, color=color, s=25, zorder=3, label=label)
    legend_handles.append(scatter)

ax.legend(handles=legend_handles, title="Class", loc="upper right")
ax.set_title("Mean entropy distribution by class")
ax.set_xlabel("Class")
ax.set_ylabel("Mean entropy (bits)")
ax.grid(True, alpha=0.3, axis="y")
plt.tight_layout()
plt.show()

# ── 2. F1 vs threshold sweep (binary: AF vs non-AF) ───────────────────────────
thresholds = np.linspace(df["mean_entropy"].min(), df["mean_entropy"].max(), 200)
f1_scores  = []
y_true_bin = (df["true_label"] == "af").astype(int)

print(f"\nAF samples     : {y_true_bin.sum()}")
print(f"Non-AF samples : {(y_true_bin == 0).sum()}")

for thresh in thresholds:
    y_pred_bin = (df["mean_entropy"] > thresh).astype(int)
    f1 = f1_score(y_true_bin, y_pred_bin, zero_division=0)
    f1_scores.append(f1)

best_idx    = np.argmax(f1_scores)
best_thresh = thresholds[best_idx]
best_f1     = f1_scores[best_idx]

fig, ax = plt.subplots(figsize=(10, 5))
ax.plot(thresholds, f1_scores, linewidth=1.5)
ax.axvline(x=best_thresh, color="red", linestyle="--",
           label=f"Best threshold = {best_thresh:.3f}  (F1 = {best_f1:.3f})")
ax.set_title("F1 score vs entropy threshold (AF vs non-AF)")
ax.set_xlabel("Entropy threshold")
ax.set_ylabel("F1 score")
ax.legend()
ax.grid(True, alpha=0.3)
plt.tight_layout()
plt.show()

print(f"\nBest entropy threshold : {best_thresh:.4f}")
print(f"Best binary F1 score   : {best_f1:.4f}")

# ── 3. Confusion matrix at best threshold ─────────────────────────────────────
y_pred_best = (df["mean_entropy"] > best_thresh).astype(int)
cm          = confusion_matrix(y_true_bin, y_pred_best)
disp        = ConfusionMatrixDisplay(cm, display_labels=["Non-AF", "AF"])
disp.plot()
plt.title(f"Confusion matrix at threshold = {best_thresh:.3f}")
plt.show()

# ── 4. Macro F1 sweep (all 4 classes) ─────────────────────────────────────────
y_true_multi  = df["true_label"]
best_macro_f1 = 0
best_ent_t    = 0

for thresh in thresholds:
    def classify(row, t=thresh):
        if row["mean_entropy"] > t:
            return "af"
        elif row["mean_entropy"] < t:
            return "normal"
        else:
            return "other"

    y_pred_multi = df.apply(classify, axis=1)
    macro_f1     = f1_score(y_true_multi, y_pred_multi,
                            average="macro", zero_division=0)
    if macro_f1 > best_macro_f1:
        best_macro_f1 = macro_f1
        best_ent_t    = thresh

print(f"\nBest macro F1          : {best_macro_f1:.4f}")
print(f"Best entropy threshold : {best_ent_t:.4f}")