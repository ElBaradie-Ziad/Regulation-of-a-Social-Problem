import numpy as np
import matplotlib.pyplot as plt

def generer_scores_risque(n_posts):
    """
    Generates risk scores (float 0-100) with:
    - VERY high probability around 0
    - Moderate peak around 50
    - Slight rise between 80 and 100
    """
    # Proportions based on your snippet
    n_bas = int(n_posts * 0.80)      # 80% Low risk
    n_mid = int(n_posts * 0.15)      # 15% Mid risk
    n_haut = n_posts - n_bas - n_mid # ~5% High risk

    # 1. Around 0 (Massive majority)
    dist_bas = np.abs(np.random.normal(loc=0, scale=12, size=n_bas))

    # 2. Peak around 50 (Smaller)
    dist_mid = np.random.normal(loc=50, scale=8, size=n_mid)

    # 3. Rise between 80 and 100 (Slight)
    dist_haut = np.random.normal(loc=92, scale=6, size=n_haut)

    # Merge
    risques = np.concatenate([dist_bas, dist_mid, dist_haut])

    # Clipping and Shuffle
    risques = np.clip(risques, 0.0, 100.0)
    np.random.shuffle(risques)

    return risques

# --- VISUALIZATION ---
n = 100_000
data = generer_scores_risque(n)

plt.figure(figsize=(10, 5))
plt.hist(data, bins=100, color='#4c72b0', edgecolor='black', alpha=0.7)

# Updated English Labels and Title
plt.title("Estimation of Incoming Posts Risk Distribution")
plt.xlabel("Risk Score")
plt.ylabel("Number of Posts")

plt.show()