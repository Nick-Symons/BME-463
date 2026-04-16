import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.metrics import f1_score, confusion_matrix, ConfusionMatrixDisplay, accuracy_score

# ── Load and diagnose ─────────────────────────────────────────────────────────
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

df["true_label"] = df["true_label"].str.lower().str.strip()

print(f"\nRecords per class:\n{df['true_label'].value_counts()}")

# ── Entropy box plot ──────────────────────────────────────────────────────
fig, ax = plt.subplots(figsize=(10, 5))
classes  = ["af", "normal", "noisy", "other"]
colors   = ["#E24B4A", "#1D9E75", "#EF9F27", "#888780"]

plot_data = [df[df["true_label"] == c]["mean_entropy"].values for c in classes]
bp = ax.boxplot(plot_data, tick_labels=classes, patch_artist=True)
for patch, color in zip(bp["boxes"], colors):
    patch.set_facecolor(color)
    patch.set_alpha(0.6)

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

"""
Removing noise classification from algorithm so no need to plot these features and tune thresholds for noisy class
Keeping here for reference 
# ── Kurtosis box plot ─────────────────────────────────────────────────────
fig, ax = plt.subplots(figsize=(10, 5))

plot_data = [df[df["true_label"] == c]["mean_kurtosis"].values for c in classes]
bp = ax.boxplot(plot_data, tick_labels=classes, patch_artist=True)
for patch, color in zip(bp["boxes"], colors):
    patch.set_facecolor(color)
    patch.set_alpha(0.6)

legend_handles = []
for i, (data, color, label) in enumerate(zip(plot_data, colors, classes)):
    x = np.random.normal(i + 1, 0.06, size=len(data))
    scatter = ax.scatter(x, data, alpha=0.9, color=color, s=25, zorder=3, label=label)
    legend_handles.append(scatter)

ax.legend(handles=legend_handles, title="Class", loc="upper right")
ax.set_title("Mean kurtosis distribution by class")
ax.set_xlabel("Class")
ax.set_ylabel("Mean kurtosis")
ax.grid(True, alpha=0.3, axis="y")
plt.tight_layout()
plt.show()

# ── Residual box plot ─────────────────────────────────────────────────────
fig, ax = plt.subplots(figsize=(10, 5))

plot_data = [df[df["true_label"] == c]["mean_residual"].values for c in classes]
bp = ax.boxplot(plot_data, tick_labels=classes, patch_artist=True)
for patch, color in zip(bp["boxes"], colors):
    patch.set_facecolor(color)
    patch.set_alpha(0.6)

legend_handles = []
for i, (data, color, label) in enumerate(zip(plot_data, colors, classes)):
    x = np.random.normal(i + 1, 0.06, size=len(data))
    scatter = ax.scatter(x, data, alpha=0.9, color=color, s=25, zorder=3, label=label)
    legend_handles.append(scatter)

ax.legend(handles=legend_handles, title="Class", loc="upper right")
ax.set_title("Mean residual distribution by class")
ax.set_xlabel("Class")
ax.set_ylabel("Mean residual")
ax.grid(True, alpha=0.3, axis="y")
plt.tight_layout()
plt.show()

# ── Stage 1a: kurtosis threshold sweep — noisy vs non-noisy ────────────────
kurt_thresholds = np.linspace(df["mean_kurtosis"].min(), df["mean_kurtosis"].max(), 200)
y_true_noisy    = (df["true_label"] == "noisy").astype(int)
kurt_f1_scores  = []

for thresh in kurt_thresholds:
    y_pred_noisy = (df["mean_kurtosis"] < thresh).astype(int)  # low kurtosis = noisy
    f1 = f1_score(y_true_noisy, y_pred_noisy, zero_division=0)
    kurt_f1_scores.append(f1)

best_kurt_idx    = np.argmax(kurt_f1_scores)
best_kurt_thresh = kurt_thresholds[best_kurt_idx]
best_kurt_f1     = kurt_f1_scores[best_kurt_idx]

fig, ax = plt.subplots(figsize=(10, 5))
ax.plot(kurt_thresholds, kurt_f1_scores, linewidth=1.5)
ax.axvline(x=best_kurt_thresh, color="red", linestyle="--",
           label=f"Best threshold = {best_kurt_thresh:.3f}  (F1 = {best_kurt_f1:.3f})")
ax.set_title("F1 score vs kurtosis threshold (noisy vs non-noisy)")
ax.set_xlabel("Kurtosis threshold")
ax.set_ylabel("F1 score")
ax.legend()
ax.grid(True, alpha=0.3)
plt.tight_layout()
plt.show()

print(f"\n-- Stage 1: Noise gate --")
print(f"Best kurtosis threshold : {best_kurt_thresh:.4f}")
print(f"Best noisy F1 score     : {best_kurt_f1:.4f}")

y_pred_noisy_best = (df["mean_kurtosis"] < best_kurt_thresh).astype(int)
cm   = confusion_matrix(y_true_noisy, y_pred_noisy_best)
disp = ConfusionMatrixDisplay(cm, display_labels=["Non-noisy", "Noisy"])
disp.plot()
plt.title(f"Noise gate confusion matrix -- kurtosis threshold = {best_kurt_thresh:.4f}")
plt.show()

# ── Stage 1b: residual threshold sweep — noisy vs non-noisy ────────────────
res_thresholds = np.linspace(df["mean_residual"].min(), df["mean_residual"].max(), 200)
y_true_noisy    = (df["true_label"] == "noisy").astype(int)
res_f1_scores  = []

for thresh in res_thresholds:
    y_pred_noisy = (df["mean_residual"] < thresh).astype(int)  # low kurtosis = noisy
    f1 = f1_score(y_true_noisy, y_pred_noisy, zero_division=0)
    res_f1_scores.append(f1)

best_res_idx    = np.argmax(res_f1_scores)
best_res_thresh = res_thresholds[best_res_idx]
best_res_f1     = res_f1_scores[best_res_idx]

fig, ax = plt.subplots(figsize=(10, 5))
ax.plot(res_thresholds, res_f1_scores, linewidth=1.5)
ax.axvline(x=best_res_thresh, color="red", linestyle="--",
           label=f"Best threshold = {best_res_thresh:.3f}  (F1 = {best_res_f1:.3f})")
ax.set_title("F1 score vs Residual threshold (noisy vs non-noisy)")
ax.set_xlabel("Residual threshold")
ax.set_ylabel("F1 score")
ax.legend()
ax.grid(True, alpha=0.3)
plt.tight_layout()
plt.show()

print(f"\n-- Stage 1: Noise gate --")
print(f"Best Residual threshold : {best_res_thresh:.4f}")
print(f"Best noisy F1 score     : {best_res_f1:.4f}")

y_pred_noisy_best = (df["mean_residual"] < best_res_thresh).astype(int)
cm   = confusion_matrix(y_true_noisy, y_pred_noisy_best)
disp = ConfusionMatrixDisplay(cm, display_labels=["Non-noisy", "Noisy"])
disp.plot()
plt.title(f"Noise gate confusion matrix -- residual threshold = {best_res_thresh:.4f}")
plt.show()

# Show confusion matrix combining both classification features
y_pred_noisy_best = ((df["mean_kurtosis"] < best_kurt_thresh) & (df["mean_residual"] < best_res_thresh)).astype(int)
cm   = confusion_matrix(y_true_noisy, y_pred_noisy_best)
disp = ConfusionMatrixDisplay(cm, display_labels=["Non-noisy", "Noisy"])
disp.plot()
plt.title(f"Noise gate confusion matrix using both kurtosis threshold = {best_kurt_thresh:.4f} and residual threshold = {best_res_thresh:.4f}")
plt.show()

# Stage 2 runs on signals that pass the kurtosis and residual noise gate
df_clean = df[(df["mean_kurtosis"] >= best_kurt_thresh) & (df["mean_residual"] >= best_res_thresh)].copy()

"""

# ── Noise guard confusion matrix ──────────────────────────────────────────────
y_true_noisy      = (df["true_label"] == "noisy").astype(int)
y_pred_noisy      = df["mean_entropy"].isna().astype(int)  # NA = caught by noise guard

cm   = confusion_matrix(y_true_noisy, y_pred_noisy)
disp = ConfusionMatrixDisplay(cm, display_labels=["Non-noisy", "Noisy"])
disp.plot()
plt.title("Noise guard confusion matrix\n(NA entropy = flagged as noisy by BPM/interval guards)")
plt.show()

noisy_f1       = f1_score(y_true_noisy, y_pred_noisy, zero_division=0)
noisy_caught   = y_pred_noisy[y_true_noisy == 1].sum()
noisy_total    = y_true_noisy.sum()
false_positives = y_pred_noisy[y_true_noisy == 0].sum()

print(f"\n-- Noise guard performance --")
print(f"Noisy signals caught    : {noisy_caught} / {noisy_total}")
print(f"False positives         : {false_positives} clean signals incorrectly flagged")
print(f"Noise guard F1          : {noisy_f1:.4f}")

# ── 3. Stage 2: entropy threshold sweep — AF vs non-AF (excluding noisy) ─────
# Only run on signals that pass the noise gate
df_clean = df.dropna(subset=["mean_entropy"]).copy()

print(f"\n--- Stage 2: AF classifier (on {len(df_clean)} clean signals) ------------")
print(f"Records per class after noise gate:\n{df_clean['true_label'].value_counts()}")

ent_thresholds = np.linspace(df_clean["mean_entropy"].min(), df_clean["mean_entropy"].max(), 200)
y_true_af      = (df_clean["true_label"] == "af").astype(int)
ent_f1_scores  = []

for thresh in ent_thresholds:
    y_pred_af = (df_clean["mean_entropy"] > thresh).astype(int)
    f1 = f1_score(y_true_af, y_pred_af, zero_division=0)
    ent_f1_scores.append(f1)

best_ent_idx    = np.argmax(ent_f1_scores)
best_ent_thresh = ent_thresholds[best_ent_idx]
best_ent_f1     = ent_f1_scores[best_ent_idx]

fig, ax = plt.subplots(figsize=(10, 5))
ax.plot(ent_thresholds, ent_f1_scores, linewidth=1.5)
ax.axvline(x=best_ent_thresh, color="red", linestyle="--",
           label=f"Best threshold = {best_ent_thresh:.3f}  (F1 = {best_ent_f1:.3f})")
ax.set_title("F1 score vs entropy threshold (AF vs non-AF, clean signals only)")
ax.set_xlabel("Entropy threshold")
ax.set_ylabel("F1 score")
ax.legend()
ax.grid(True, alpha=0.3)
plt.tight_layout()
plt.show()

print(f"Best entropy threshold  : {best_ent_thresh:.4f}")
print(f"Best AF F1 score        : {best_ent_f1:.4f}")

# Confusion matrix for AF classifier
y_pred_af_best = (df_clean["mean_entropy"] > best_ent_thresh).astype(int)
cm   = confusion_matrix(y_true_af, y_pred_af_best)
disp = ConfusionMatrixDisplay(cm, display_labels=["Non-AF", "AF"])
disp.plot()
plt.title(f"AF classifier confusion matrix — entropy threshold = {best_ent_thresh:.3f}")
plt.show()

# ── 4. Two-threshold entropy sweep (AF / Other / Normal) ─────────────────────
print(f"\n-- Combined macro F1 (all 4 classes) --")

ent_low_thresholds  = np.linspace(df_clean["mean_entropy"].min(),
                                   df_clean["mean_entropy"].max(), 50)
ent_high_thresholds = np.linspace(df_clean["mean_entropy"].min(),
                                   df_clean["mean_entropy"].max(), 50)

best_macro_f1  = 0
best_low_t     = 0
best_high_t    = 0

for low_t in ent_low_thresholds:
    for high_t in ent_high_thresholds:
        if high_t <= low_t:
            continue

        def classify_combined(row, lt=low_t, ht=high_t):
            if pd.isna(row["mean_entropy"]):  
                return "noisy"
            elif row["mean_entropy"] > ht:
                return "af"
            elif row["mean_entropy"] < lt:
                return "normal"
            else:
                return "other"

        y_pred   = df.apply(classify_combined, axis=1)
        macro_f1 = f1_score(df["true_label"], y_pred,
                            average="macro", zero_division=0)

        if macro_f1 > best_macro_f1:
            best_macro_f1 = macro_f1
            best_low_t    = low_t
            best_high_t   = high_t

print(f"Low entropy threshold   : {best_low_t:.4f}  (below = Normal)")
print(f"High entropy threshold  : {best_high_t:.4f}  (above = AF)")
print(f"Combined macro F1       : {best_macro_f1:.4f}")

# ── Final confusion matrix ────────────────────────────────────────────────────
y_pred_final = df.apply(
    lambda row: classify_combined(row, best_low_t, best_high_t),
    axis=1
)

all_classes = ["af", "normal", "noisy", "other"]
cm   = confusion_matrix(df["true_label"], y_pred_final, labels=all_classes)
disp = ConfusionMatrixDisplay(cm, display_labels=all_classes)
disp.plot()
plt.title("Full confusion matrix -- all 4 classes")
plt.show()

# ── Visualise the two entropy thresholds on the distribution ──────────────────
fig, ax = plt.subplots(figsize=(10, 5))

for data, color, label in zip(
        [df_clean[df_clean["true_label"] == c]["mean_entropy"].values for c in classes],
        colors, classes):
    ax.scatter([label] * len(data), data, alpha=0.7, color=color, s=25, zorder=3, label=label)

ax.axhline(y=best_low_t,  color="blue", linestyle="--", linewidth=1.2,
           label=f"Low threshold  = {best_low_t:.3f}  (Normal below)")
ax.axhline(y=best_high_t, color="red",  linestyle="--", linewidth=1.2,
           label=f"High threshold = {best_high_t:.3f}  (AF above)")
ax.set_title("Entropy distribution with classification thresholds")
ax.set_xlabel("Class")
ax.set_ylabel("Mean entropy (bits)")
ax.legend()
ax.grid(True, alpha=0.3, axis="y")
plt.tight_layout()
plt.show()

# ── Total accuracy ────────────────────────────────────────────────────────────
total_accuracy = accuracy_score(df["true_label"], y_pred_final)
per_class_accuracy = {
    cls: accuracy_score(
        (df["true_label"] == cls).astype(int),
        (y_pred_final == cls).astype(int)
    )
    for cls in all_classes
}
 
print(f"\n{'='*50}")
print(f"  FINAL RESULTS SUMMARY")
print(f"{'='*50}")
print(f"  Total accuracy        : {total_accuracy:.4f}  ({total_accuracy*100:.2f}%)")
print(f"  Combined macro F1     : {best_macro_f1:.4f}")
print(f"\n  Per-class accuracy:")
for cls, acc in per_class_accuracy.items():
    print(f"    {cls:<10}: {acc:.4f}  ({acc*100:.2f}%)")
print(f"{'='*50}")
