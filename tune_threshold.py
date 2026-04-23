import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.svm import SVC
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import StratifiedKFold, GridSearchCV, cross_val_score
from sklearn.metrics import (confusion_matrix, ConfusionMatrixDisplay,
                             classification_report)
from sklearn.inspection import permutation_importance
from sklearn.metrics import make_scorer

# ── PhysioNet F1 score ────────────────────────────────────────────────────────
def physionet_f1_score(y_true, y_pred):
    classes = {"normal": "n", "af": "a", "other": "o", "noisy": "p"}
    f1s     = {}
    for cls, suffix in classes.items():
        tp         = sum(1 for t, p in zip(y_true, y_pred) if t == cls and p == cls)
        total_true = sum(1 for t in y_true if t == cls)
        total_pred = sum(1 for p in y_pred if p == cls)
        denom      = total_true + total_pred
        f1s[cls]   = (2 * tp / denom) if denom > 0 else 0.0
    return sum(f1s.values()) / 4, f1s

def physionet_scorer_fn(y_true, y_pred):
    score, _ = physionet_f1_score(list(y_true), list(y_pred))
    return score

def physionet_scorer_pi(estimator, X, y):
    y_pred = estimator.predict(X)
    score, _ = physionet_f1_score(list(y), list(y_pred))
    return score

physionet_score = make_scorer(physionet_scorer_fn)

# ── Load data ─────────────────────────────────────────────────────────────────
filepath = "C:\\Users\\timmy\\OneDrive\\TCD Engineering\\SS - Semester 2\\Computers in Medicine\\Project - Single Lead AF Classification\\BME-463\\results.csv";
try:
    df = pd.read_csv(filepath, sep="\t")
    if "true_label" not in df.columns:
        df = pd.read_csv(filepath, sep=",")
except:
    df = pd.read_csv(filepath, sep=",")

df["true_label"] = df["true_label"].str.lower().str.strip()

print("Columns found    :", df.columns.tolist())
print("Shape            :", df.shape)
print(f"\nRecords per class:\n{df['true_label'].value_counts()}")

# ── Features ──────────────────────────────────────────────────────────────────
features = ["mean_entropy", "mean_p_wave_amplitude", "mean_kurtosis",
            "mean_RMSSD", "mean_template_diff"]

df_ml = df.dropna(subset=features + ["true_label"]).copy()
print(f"\nSamples after dropping NA: {len(df_ml)}")
print(f"Records per class after dropna:\n{df_ml['true_label'].value_counts()}")

X = df_ml[features].values
y = df_ml["true_label"].values

all_classes = ["af", "normal", "noisy", "other"]
colors      = ["#E24B4A", "#1D9E75", "#EF9F27", "#888780"]

# ── Box plots ─────────────────────────────────────────────────────────────────
feature_plots = [
    ("mean_entropy",          "Mean entropy (bits)",  "Mean entropy distribution by class"),
    ("mean_p_wave_amplitude", "P-wave amplitude",     "P-wave amplitude distribution by class"),
    ("mean_kurtosis",         "Kurtosis",             "Kurtosis distribution by class"),
    ("mean_RMSSD",            "RMSSD (s)",            "RMSSD distribution by class"),
    ("mean_template_diff",    "Template difference",  "Template difference distribution by class"),
]

for col, ylabel, title in feature_plots:
    if col not in df.columns:
        print(f"Skipping {col} -- not in CSV")
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
    print(f"  {f:<25} mean={scaler.mean_[i]:.6f}  std={scaler.scale_[i]:.6f}")

# ── Grid search ───────────────────────────────────────────────────────────────
print(f"\n-- Grid search over C (linear kernel, 5-fold stratified CV) --")

param_grid = {"C": [0.01, 0.1, 1.0, 10.0]}
cv         = StratifiedKFold(n_splits=5, shuffle=True, random_state=42)

grid_search = GridSearchCV(
    SVC(kernel="linear", class_weight="balanced"),
    param_grid,
    cv=cv,
    scoring=physionet_score,
    n_jobs=-1,
    verbose=2
)
grid_search.fit(X_scaled, y)

print(f"\nGrid search results (PhysioNet F1):")
for mean, std, params in zip(
        grid_search.cv_results_["mean_test_score"],
        grid_search.cv_results_["std_test_score"],
        grid_search.cv_results_["params"]):
    print(f"  C={params['C']:<6} -> PhysioNet F1 = {mean:.4f} +/- {std:.4f}")

best_C = grid_search.best_params_["C"]
print(f"\nBest C              : {best_C}")
print(f"Best CV PhysioNet F1: {grid_search.best_score_:.4f}")

# ── Train final model ─────────────────────────────────────────────────────────
svm = SVC(kernel="linear", C=best_C, class_weight="balanced")
svm.fit(X_scaled, y)

y_pred_train        = svm.predict(X_scaled)
train_f1, train_cls = physionet_f1_score(list(y), list(y_pred_train))

print(f"\n-- Overfitting check --")
print(f"Training PhysioNet F1 : {train_f1:.4f}")
print(f"CV PhysioNet F1       : {grid_search.best_score_:.4f}")
print(f"Difference            : {train_f1 - grid_search.best_score_:.4f}  ", end="")
if train_f1 - grid_search.best_score_ > 0.15:
    print("(WARNING: likely overfitting)")
else:
    print("(OK)")

# ── Per-fold CV scores ────────────────────────────────────────────────────────
fold_scores = cross_val_score(
    SVC(kernel="linear", C=best_C, class_weight="balanced"),
    X_scaled, y, cv=cv, scoring=physionet_score, n_jobs=-1
)
print(f"\n-- Per-fold CV scores (PhysioNet F1) --")
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
print(f"\n-- Permutation importance (PhysioNet F1) --")

perm_imp   = permutation_importance(
    svm, X_scaled, y,
    n_repeats=10,
    random_state=42,
    scoring=physionet_scorer_pi
)
sorted_idx = perm_imp.importances_mean.argsort()[::-1]

print(f"\n  {'Feature':<25} {'Mean drop':>10} {'Std':>8} {'Signal':>10}")
print(f"  {'-'*55}")
for i in sorted_idx:
    mean = perm_imp.importances_mean[i]
    std  = perm_imp.importances_std[i]
    snr  = mean / std if std > 0 else 0
    flag = "DROP?" if snr < 1.0 else "OK"
    print(f"  {features[i]:<25} {mean:>10.4f} {std:>8.4f} {flag:>10}")

fig, ax = plt.subplots(figsize=(8, 5))
ax.barh(
    [features[i] for i in sorted_idx],
    perm_imp.importances_mean[sorted_idx],
    xerr=perm_imp.importances_std[sorted_idx],
    color=colors[:len(features)],
    alpha=0.7
)
ax.axvline(x=0, color="black", linewidth=0.8, linestyle="--")
ax.set_title("Permutation importance (PhysioNet F1 drop)")
ax.set_xlabel("Mean PhysioNet F1 decrease")
ax.grid(True, alpha=0.3, axis="x")
plt.tight_layout()
plt.show()

# ── 2D scatter: entropy vs P-wave amplitude ───────────────────────────────────
fig, ax = plt.subplots(figsize=(10, 7))
for color, label in zip(colors, all_classes):
    subset = df_ml[df_ml["true_label"] == label]
    ax.scatter(subset["mean_entropy"], subset["mean_p_wave_amplitude"],
               color=color, alpha=0.7, s=40, label=label, zorder=3)
ax.set_title("Entropy vs P-wave amplitude by class")
ax.set_xlabel("Mean entropy (bits)")
ax.set_ylabel("Mean P-wave amplitude")
ax.legend(title="Class")
ax.grid(True, alpha=0.3)
plt.tight_layout()
plt.show()

# ── C++ deployment parameters ─────────────────────────────────────────────────
print(f"\n-- Scaler parameters for C++ --")
print(f"float scaler_mean[] = {{", end="")
print(", ".join([f"{m:.6f}f" for m in scaler.mean_]), end="")
print("};")
print(f"float scaler_std[]  = {{", end="")
print(", ".join([f"{s:.6f}f" for s in scaler.scale_]), end="")
print("};")

print(f"\n-- SVM weight vector for C++ (linear kernel) --")
print(f"// Classes: {list(svm.classes_)}")
for i, coef in enumerate(svm.coef_):
    print(f"float w_{i}[] = {{", end="")
    print(", ".join([f"{c:.6f}f" for c in coef]), end="")
    print("};")
print(f"float intercept[] = {{", end="")
print(", ".join([f"{b:.6f}f" for b in svm.intercept_]), end="")
print("};")

# ── Final summary ─────────────────────────────────────────────────────────────
y_pred_final     = svm.predict(X_scaled)
overall, per_cls = physionet_f1_score(list(y), list(y_pred_final))

print(f"\n{'='*50}")
print(f"  SVM RESULTS SUMMARY")
print(f"{'='*50}")
print(f"  Kernel              : Linear")
print(f"  Best C              : {best_C}")
print(f"  Training PhysioNet F1 : {train_f1:.4f}")
print(f"  CV PhysioNet F1       : {fold_scores.mean():.4f} +/- {fold_scores.std():.4f}")
print(f"  Features used         : {features}")
print(f"  F1n (Normal)          : {per_cls.get('normal', 0):.4f}")
print(f"  F1a (AF)              : {per_cls.get('af',     0):.4f}")
print(f"  F1o (Other)           : {per_cls.get('other',  0):.4f}")
print(f"  F1p (Noisy)           : {per_cls.get('noisy',  0):.4f}")
print(f"  Overall PhysioNet F1  : {overall:.4f}")
print(f"{'='*50}")