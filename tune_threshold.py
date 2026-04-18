import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.svm import SVC
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import StratifiedKFold, GridSearchCV, cross_val_score
from sklearn.metrics import (f1_score, confusion_matrix, ConfusionMatrixDisplay,
                             classification_report, accuracy_score)
from sklearn.inspection import permutation_importance

# ── Load data ─────────────────────────────────────────────────────────────────
try:
    df = pd.read_csv("results.csv", sep="\t")
    if "true_label" not in df.columns:
        df = pd.read_csv("results.csv", sep=",")
except:
    df = pd.read_csv("results.csv", sep=",")

df["true_label"] = df["true_label"].str.lower().str.strip()

print("Columns found    :", df.columns.tolist())
print("Shape            :", df.shape)
print(f"\nRecords per class:\n{df['true_label'].value_counts()}")

# ── Prepare features ──────────────────────────────────────────────────────────
features = ["mean_entropy", "mean_p_wave_amplitude", "mean_kurtosis", "mean_residual"]

df_ml = df.dropna(subset=features + ["true_label"]).copy()
print(f"\nSamples after dropping NA: {len(df_ml)}")
print(f"Records per class after dropna:\n{df_ml['true_label'].value_counts()}")

X = df_ml[features].values
y = df_ml["true_label"].values

all_classes = ["af", "normal", "noisy", "other"]

# ── Box plots for all features ────────────────────────────────────────────────
all_classes = ["af", "normal", "noisy", "other"]
colors      = ["#E24B4A", "#1D9E75", "#EF9F27", "#888780"]

feature_plots = [
    ("mean_entropy",          "Mean entropy (bits)",    "Mean entropy distribution by class"),
    ("mean_p_wave_amplitude", "P-wave amplitude",       "P-wave amplitude distribution by class"),
    ("mean_kurtosis",         "Kurtosis",               "Kurtosis distribution by class"),
    ("mean_residual",         "HF residual",            "HF residual distribution by class"),
]

for col, ylabel, title in feature_plots:
    if col not in df.columns:
        print(f"Skipping {col} — not in CSV")
        continue

    fig, ax = plt.subplots(figsize=(10, 5))

    plot_data = [df[df["true_label"] == c][col].dropna().values for c in all_classes]

    bp = ax.boxplot(plot_data, tick_labels=all_classes, patch_artist=True)
    for patch, color in zip(bp["boxes"], colors):
        patch.set_facecolor(color)
        patch.set_alpha(0.6)

    legend_handles = []
    for i, (data, color, label) in enumerate(zip(plot_data, colors, all_classes)):
        x = np.random.normal(i + 1, 0.06, size=len(data))
        scatter = ax.scatter(x, data, alpha=0.9, color=color,
                             s=25, zorder=3, label=label)
        legend_handles.append(scatter)

    ax.legend(handles=legend_handles, title="Class", loc="upper right")
    ax.set_title(title)
    ax.set_xlabel("Class")
    ax.set_ylabel(ylabel)
    ax.grid(True, alpha=0.3, axis="y")
    plt.tight_layout()
    plt.show()

# ── Scale features ────────────────────────────────────────────────────────────
scaler   = StandardScaler()
X_scaled = scaler.fit_transform(X)

print(f"\n-- Feature scaling --")
for i, f in enumerate(features):
    print(f"  {f:<20} mean={scaler.mean_[i]:.4f}  std={scaler.scale_[i]:.4f}")

# ── Grid search over C with stratified CV ─────────────────────────────────────
print(f"\n-- Grid search over C (linear kernel, 5-fold stratified CV) --")

param_grid = {"C": [0.01, 0.1, 1.0, 10.0]}
cv         = StratifiedKFold(n_splits=5, shuffle=True, random_state=42)

grid_search = GridSearchCV(
    SVC(kernel="linear", class_weight="balanced"),
    param_grid,
    cv=cv,
    scoring="f1_macro",
    n_jobs=-1,   # use all CPU cores
    verbose=1
)
grid_search.fit(X_scaled, y)

print(f"\nGrid search results:")
for mean, std, params in zip(
        grid_search.cv_results_["mean_test_score"],
        grid_search.cv_results_["std_test_score"],
        grid_search.cv_results_["params"]):
    print(f"  C={params['C']:<6} -> macro F1 = {mean:.4f} +/- {std:.4f}")

best_C = grid_search.best_params_["C"]
print(f"\nBest C : {best_C}")
print(f"Best CV macro F1 : {grid_search.best_score_:.4f}")

# ── Train final model on full dataset with best C ─────────────────────────────
svm = SVC(kernel="linear", C=best_C, class_weight="balanced")
svm.fit(X_scaled, y)

y_pred_train = svm.predict(X_scaled)
train_f1     = f1_score(y, y_pred_train, average="macro", zero_division=0)

print(f"\n-- Overfitting check --")
print(f"Training macro F1 : {train_f1:.4f}")
print(f"CV macro F1       : {grid_search.best_score_:.4f}")
print(f"Difference        : {train_f1 - grid_search.best_score_:.4f}  ",
      end="")
if train_f1 - grid_search.best_score_ > 0.15:
    print("(WARNING: likely overfitting — consider lower C or more data)")
else:
    print("(OK)")

# ── Per-fold CV scores ────────────────────────────────────────────────────────
fold_scores = cross_val_score(
    SVC(kernel="linear", C=best_C, class_weight="balanced"),
    X_scaled, y, cv=cv, scoring="f1_macro"
)
print(f"\n-- Per-fold CV scores --")
for i, score in enumerate(fold_scores):
    print(f"  Fold {i+1}: {score:.4f}")
print(f"  Mean  : {fold_scores.mean():.4f}")
print(f"  Std   : {fold_scores.std():.4f}")

# ── Classification report ─────────────────────────────────────────────────────
print(f"\n-- Classification report (training set) --")
print(classification_report(y, y_pred_train, zero_division=0))

# ── Confusion matrix ──────────────────────────────────────────────────────────
cm   = confusion_matrix(y, y_pred_train, labels=all_classes)
disp = ConfusionMatrixDisplay(cm, display_labels=all_classes)
disp.plot()
plt.title(f"SVM confusion matrix (training set)\nC={best_C}, kernel=linear")
plt.show()

# ── Permutation importance ────────────────────────────────────────────────────
print(f"\n-- Permutation importance --")
print(f"(shuffling each feature 10 times, measuring macro F1 drop)")

perm_imp = permutation_importance(
    svm, X_scaled, y,
    n_repeats=10,
    random_state=42,
    scoring="f1_macro"
)

# Sort by mean importance
sorted_idx = perm_imp.importances_mean.argsort()[::-1]

print(f"\n  {'Feature':<20} {'Mean drop':>10} {'Std':>8} {'Signal':>10}")
print(f"  {'-'*50}")
for i in sorted_idx:
    mean = perm_imp.importances_mean[i]
    std  = perm_imp.importances_std[i]
    snr  = mean / std if std > 0 else 0
    flag = "DROP?" if snr < 1.0 else "OK"
    print(f"  {features[i]:<20} {mean:>10.4f} {std:>8.4f} {flag:>10}")

# Plot permutation importance
fig, ax = plt.subplots(figsize=(8, 5))
ax.barh(
    [features[i] for i in sorted_idx],
    perm_imp.importances_mean[sorted_idx],
    xerr=perm_imp.importances_std[sorted_idx],
    color=["#E24B4A", "#1D9E75", "#EF9F27", "#888780"][:len(features)],
    alpha=0.7
)
ax.axvline(x=0, color="black", linewidth=0.8, linestyle="--")
ax.set_title("Permutation importance\n(mean F1 drop when feature is shuffled)")
ax.set_xlabel("Mean macro F1 decrease")
ax.grid(True, alpha=0.3, axis="x")
plt.tight_layout()
plt.show()

# ── 2D scatter: entropy vs P-wave score coloured by class ─────────────────────
colors = ["#E24B4A", "#1D9E75", "#EF9F27", "#888780"]

fig, ax = plt.subplots(figsize=(10, 7))
for color, label in zip(colors, all_classes):
    subset = df_ml[df_ml["true_label"] == label]
    ax.scatter(subset["mean_entropy"], subset["mean_p_wave_amplitude"],
               color=color, alpha=0.7, s=40, label=label, zorder=3)

ax.set_title("Entropy vs P-wave score by class")
ax.set_xlabel("Mean entropy (bits)")
ax.set_ylabel("P-wave score [0, 1]")
ax.legend(title="Class")
ax.grid(True, alpha=0.3)
plt.tight_layout()
plt.show()

# ── Print scaler + SVM weights for C++ deployment ────────────────────────────
print(f"\n-- Scaler parameters for C++ --")
print(f"float scaler_mean[] = {{", end="")
print(", ".join([f"{m:.6f}f" for m in scaler.mean_]), end="")
print("};")
print(f"float scaler_std[]  = {{", end="")
print(", ".join([f"{s:.6f}f" for s in scaler.scale_]), end="")
print("};")

print(f"\n-- SVM weight vector for C++ (linear kernel) --")
print(f"// Classes: {list(svm.classes_)}")
print(f"// One row per binary classifier (one-vs-one)")
for i, coef in enumerate(svm.coef_):
    print(f"float w_{i}[] = {{", end="")
    print(", ".join([f"{c:.6f}f" for c in coef]), end="")
    print("};")
print(f"float intercept[] = {{", end="")
print(", ".join([f"{b:.6f}f" for b in svm.intercept_]), end="")
print("};")

# ── Final summary ─────────────────────────────────────────────────────────────
print(f"\n{'='*50}")
print(f"  SVM RESULTS SUMMARY")
print(f"{'='*50}")
print(f"  Kernel              : Linear")
print(f"  Best C              : {best_C}")
print(f"  Training macro F1   : {train_f1:.4f}")
print(f"  CV macro F1         : {fold_scores.mean():.4f} +/- {fold_scores.std():.4f}")
print(f"  Features used       : {features}")
print(f"{'='*50}")