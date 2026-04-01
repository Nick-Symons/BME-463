# -*- coding: utf-8 -*-

import wfdb
import matplotlib.pyplot as plt
import numpy as np
import os

num_files = 20
d = 'challenge-2017/training'
d_dir = './' + d + '_local/'

# -------------------------
# Record lists (trimmed to num_files)
# -------------------------

r_af = [
"A00/A00004","A00/A00005","A00/A00009","A00/A00015","A00/A00027",
"A00/A00054","A00/A00067","A00/A00071","A00/A00087","A00/A00090",
"A00/A00101","A00/A00102","A00/A00107","A00/A00128","A00/A00132",
"A00/A00137","A00/A00141","A00/A00156","A00/A00208","A00/A00216"
][:num_files]

r_normal = [
"A00/A00001","A00/A00002","A00/A00003","A00/A00006","A00/A00007",
"A00/A00010","A00/A00011","A00/A00012","A00/A00014","A00/A00016",
"A00/A00018","A00/A00019","A00/A00021","A00/A00025","A00/A00026",
"A00/A00028","A00/A00031","A00/A00032","A00/A00033","A00/A00035"
][:num_files]

r_noisy = [
"A00/A00022","A00/A00034","A00/A00056","A00/A00106","A00/A00125",
"A00/A00139","A00/A00164","A00/A00196","A00/A00201","A00/A00205",
"A00/A00307","A00/A00370","A00/A00445","A00/A00474","A00/A00524",
"A00/A00537","A00/A00585","A00/A00591","A00/A00619","A00/A00629"
][:num_files]

r_other = [
"A00/A00008","A00/A00013","A00/A00017","A00/A00020","A00/A00023",
"A00/A00024","A00/A00029","A00/A00030","A00/A00038","A00/A00041",
"A00/A00043","A00/A00047","A00/A00055","A00/A00058","A00/A00061",
"A00/A00065","A00/A00069","A00/A00070","A00/A00074","A00/A00075"
][:num_files]

# -------------------------
# Concatenate record arrays
# -------------------------

r = r_af + r_normal + r_noisy + r_other

categories = ["af", "normal", "noisy", "other"]

# -------------------------
# Process records
# -------------------------

for i, rec in enumerate(r):

    # Determine category folder
    cat_index = i // num_files
    cat_name = categories[cat_index]
    save_dir = f'./output/{cat_name}/'

    os.makedirs(save_dir, exist_ok=True)

    # 1. Download record
    wfdb.dl_database(d, dl_dir=d_dir, records=[rec])

    # 2. Read record
    record = wfdb.rdrecord(d_dir + rec, physical=False)

    # 3. Save MLII ADC values
    record = wfdb.rdrecord(d_dir + rec, physical=False)

    mlii_adc = record.d_signal[:, 0]

    min_val = min(mlii_adc)
    mlii_adc = [item - min_val for item in mlii_adc]

    max_val = max(mlii_adc)
    mlii_adc = [item * (4095 / max_val) for item in mlii_adc]

    filename = save_dir + 'record_' + rec.replace("/", "_") + '_mlii_adc.txt'
    np.savetxt(filename, mlii_adc, fmt='%04d')