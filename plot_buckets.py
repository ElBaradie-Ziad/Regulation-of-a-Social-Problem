import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# 1. Configuration
csv_file = 'data_6jours.csv'
output_dir = 'graphiques_output'

# Création du dossier de sortie s'il n'existe pas
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

# 2. Chargement des données
try:
    df = pd.read_csv(csv_file)
except FileNotFoundError:
    print(f"Erreur : Impossible de trouver {csv_file}. Avez-vous lancé la simulation C ?")
    exit()

print(f"Génération des graphiques pour {len(df)} instantanés...")

# 3. Boucle sur chaque ligne du CSV (chaque étape de temps)
for index, row in df.iterrows():
    time_val = int(row['Day'])
    
    
     # Extraction des données (toutes les colonnes sauf 'Day' et 'Total_Posts')
    buckets_data = row.drop(['Day', 'Total_Posts']).values.astype(int)
    buckets_indices = np.arange(len(buckets_data))
    
    # --- CRÉATION DU GRAPHIQUE ---
    plt.figure(figsize=(12, 6))
    
    # Couleurs : Gris pour la zone compressée, Vert pour la zone haute précision
    # On sait que le split est autour du bucket 26 (pour Split @ 45)
    split_bucket = 26
    colors = ['gray' if i < split_bucket else '#2ca02c' for i in buckets_indices]
    
    plt.bar(buckets_indices, buckets_data, color=colors, width=1.0, edgecolor='none')
    
    # Décoration
    total_posts = buckets_data.sum()
    plt.title(f"Distribution du Stockage à T = {time_val} secondes\n(Total: {total_posts} posts)", fontsize=14, fontweight='bold')
    plt.xlabel("Bucket ID (0-255)", fontsize=12)
    plt.ylabel("Nombre de Posts", fontsize=12)
    plt.xlim(0, 255)
    
    # On fixe l'échelle Y pour voir l'évolution (optionnel, sinon laisser matplotlib adapter)
    # plt.ylim(0, 8000) 
    plt.grid(axis='y', alpha=0.3, linestyle='--')
    
    # Ligne de séparation
    plt.axvline(x=split_bucket, color='red', linestyle='--', linewidth=1.5, alpha=0.7)
    
    if index == 0: # Annotations seulement sur le premier ou tous, au choix
        plt.text(10, plt.ylim()[1]*0.8, "Zone\nCompressée", color='gray', ha='center', fontsize=9)
        plt.text(60, plt.ylim()[1]*0.8, "Zone Haute Précision", color='green', fontsize=9)

    # --- SAUVEGARDE ---
    filename = f"etat_T{time_val:03d}.png" # ex: etat_T005.png, etat_T010.png
    filepath = os.path.join(output_dir, filename)
    
    plt.savefig(filepath)
    plt.close() # Important pour libérer la mémoire entre chaque image
    
    print(f"   -> Image générée : {filepath}")

print("\nTerminé ! Tous les graphiques sont dans le dossier 'graphiques_output'.")