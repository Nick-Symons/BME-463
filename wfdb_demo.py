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
"A01/A01005","A01/A01013","A01/A01019","A01/A01024","A01/A01029",
"A01/A01041","A01/A01042","A01/A01045","A01/A01052","A01/A01075",
"A01/A01076","A01/A01079","A01/A01087","A01/A01092","A01/A01104",
"A01/A01129","A01/A01153","A01/A01163","A01/A01171","A01/A01192"
][:num_files]

r_normal = [
"A01/A01010","A01/A01011","A01/A01012","A01/A01014","A01/A01015",
"A01/A01016","A01/A01017","A01/A01018","A01/A01020","A01/A01021",
"A01/A01022","A01/A01023","A01/A01026","A01/A01028","A01/A01032",
"A01/A01033","A01/A01034","A01/A01035","A01/A01038","A01/A01039"
][:num_files]

r_noisy = [
"A01/A01005","A01/A01013","A01/A01019","A01/A01024","A01/A01029",
"A01/A01041","A01/A01042","A01/A01045","A01/A01052","A01/A01075",
"A01/A01076","A01/A01079","A01/A01087","A01/A01092","A01/A01104",
"A01/A01129","A01/A01153","A01/A01163","A01/A01171","A01/A01192"
][:num_files]

r_other = [
"A01/A01040","A01/A01043","A01/A01049","A01/A01057","A01/A01065",
"A01/A01068","A01/A01071","A01/A01072","A01/A01073","A01/A01078",
"A01/A01081","A01/A01084","A01/A01088","A01/A01091","A01/A01093",
"A01/A01094","A01/A01095","A01/A01099","A01/A01100","A01/A01103"
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