#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h> // Nécessaire pour log(), cos(), sqrt() -> compiler avec -lm

// ==========================================
// 1. CONFIGURATION & STRUCTURES MÉMOIRE
// ==========================================

#define NUM_INSERTS 90000000 // Simulation étendue à 60 Millions (comme Python)
#define CAPACITY    10000000 // Capacité RAM stricte (10M)
#define MAX_PRIO    255      // Buckets 0 à 255 (uint8_t est parfait)

// --- MÉMOIRE COMPACTE (SoA) ---
int* data_ids;      
uint8_t* data_prios; 
int* data_next;     

// --- INDEXATION & STOCKAGE ---
// Il nous faut 256 bits pour couvrir 0-255. 
// 256 bits / 64 bits par long = 4 longs.
uint64_t bitmap[4] = {0}; 
int      heads[MAX_PRIO + 1]; 
int      tails[MAX_PRIO + 1]; 

// --- GESTIONNAIRE DE MÉMOIRE INTERNE ---
int      free_head = 0;    
int      current_count = 0; 

// ==========================================
// 2. LOGIQUE BITMAP (O(1) CPU Hardware)
// ==========================================

static inline void bitmap_set(int p) {
    bitmap[p / 64] |= (1ULL << (p % 64));
}

static inline void bitmap_clear(int p) {
    bitmap[p / 64] &= ~(1ULL << (p % 64));
}

// Trouve la priorité MAX (Scan descendant 3 -> 0)
static inline int get_max_prio() {
    for (int i = 3; i >= 0; i--) {
        if (bitmap[i]) {
            return (i * 64) + (63 - __builtin_clzll(bitmap[i]));
        }
    }
    return -1;
}

// Trouve la priorité MIN (Scan montant 0 -> 3)
static inline int get_min_prio() {
    for (int i = 0; i < 4; i++) {
        if (bitmap[i]) {
            return (i * 64) + __builtin_ctzll(bitmap[i]);
        }
    }
    return -1;
}

// ==========================================
// 3. NOUVELLES FONCTIONS (GÉNÉRATION & BUCKETING)
// ==========================================

// Générateur de nombres aléatoires selon une Loi Normale (Gaussienne)
// Algorithme de Box-Muller
double rand_normal(double mean, double stddev) {
    static double n2 = 0.0;
    static int n2_cached = 0;
    
    if (!n2_cached) {
        double u1 = (double)rand() / RAND_MAX;
        double u2 = (double)rand() / RAND_MAX;
        double r = sqrt(-2.0 * log(u1));
        double theta = 2.0 * M_PI * u2;
        double n1 = r * cos(theta);
        n2 = r * sin(theta);
        n2_cached = 1;
        return mean + (n1 * stddev);
    } else {
        n2_cached = 0;
        return mean + (n2 * stddev);
    }
}

// Reproduction exacte de la distribution Python
double generate_risk_score() {
    double p = (double)rand() / RAND_MAX;
    double risk;

    if (p < 0.80) {
        // 80% Low Risk (autour de 0) - abs() pour l'asymétrie
        risk = fabs(rand_normal(0.0, 12.0));
    } else if (p < 0.95) {
        // 15% Mid Risk (autour de 50)
        risk = rand_normal(50.0, 8.0);
    } else {
        // 5% High Risk (autour de 92)
        risk = rand_normal(92.0, 6.0);
    }

    // Clipping entre 0 et 100
    if (risk < 0.0) return 0.0;
    if (risk > 100.0) return 100.0;
    return risk;
}

// Conversion Risque -> Bucket avec stratégie "Split @ 45"
// Entrée: Float 0-100
// Sortie: Int 0-255
int risk_to_bucket_optimized(double risk) {
    // Paramètres définis dans l'analyse Python
    const double SPLIT_RISK = 45.0;
    const int N_BUCKETS = 255;
    const double RATIO_FOCUS = 0.90; // 90% des buckets pour le haut
    
    // Bucket seuil (environ 25)
    // buckets 0 à 25 pour la zone basse
    // buckets 26 à 255 pour la zone haute
    int bucket_threshold = (int)(N_BUCKETS * (1.0 - RATIO_FOCUS));

    if (risk <= SPLIT_RISK) {
        // Zone Compressée (0 - 45) -> Buckets (0 - 25)
        return (int)((risk / SPLIT_RISK) * bucket_threshold);
    } else {
        // Zone Haute Précision (45 - 100) -> Buckets (26 - 255)
        double range_risk = 100.0 - SPLIT_RISK;           // 55
        double range_bucket = N_BUCKETS - bucket_threshold; // 230
        
        int b = bucket_threshold + 1 + (int)((risk - SPLIT_RISK) / range_risk * (range_bucket - 1));
        
        // Petite sécurité pour ne pas dépasser 255
        if (b > 255) return 255;
        return b;
    }
}

// ==========================================
// 4. SYSTÈME & MOTEUR (Inchangé sauf Init)
// ==========================================

void init_system() {
    data_ids   = malloc(sizeof(int) * CAPACITY);
    data_prios = malloc(sizeof(uint8_t) * CAPACITY);
    data_next  = malloc(sizeof(int) * CAPACITY);

    if (!data_ids || !data_prios || !data_next) {
        printf("ERREUR CRITIQUE: Mémoire insuffisante.\n");
        exit(1);
    }

    for (int i = 0; i < CAPACITY - 1; i++) data_next[i] = i + 1;
    data_next[CAPACITY - 1] = -1;
    free_head = 0;

    for (int i = 0; i <= MAX_PRIO; i++) {
        heads[i] = -1;
        tails[i] = -1;
    }
}

static inline int alloc_slot() {
    int slot = free_head;
    free_head = data_next[slot];
    return slot;
}

static inline void free_slot(int slot) {
    data_next[slot] = free_head;
    free_head = slot;
}

static inline int pop_head_internal(int p) {
    int slot = heads[p];
    int next = data_next[slot];
    if (next != -1) __builtin_prefetch(&data_next[next], 0, 3);

    heads[p] = next;
    if (heads[p] == -1) {
        tails[p] = -1;
        bitmap_clear(p);
    }
    current_count--;
    return slot;
}

// --- PUSH MODIFIÉ POUR SUPPRESSION ---
void push(int id, int bucket) {
    // Si plein, on supprime le plus faible
    if (current_count >= CAPACITY) {
        int min_bucket = get_min_prio();
        
        // Si le nouveau bucket est plus petit ou égal au minimum actuel stocké,
        // ça ne sert à rien de l'insérer (il se ferait éjecter tout de suite).
        // Note: Dans notre logique split@45, c'est rare car les buckets bas sont vite vides.
        if (bucket <= min_bucket) return; 

        // On éjecte le min pour faire de la place
        int dead = pop_head_internal(min_bucket);
        free_slot(dead);
    }

    int slot = alloc_slot();
    data_ids[slot] = id;
    data_prios[slot] = (uint8_t)bucket;
    data_next[slot] = -1;

    if (tails[bucket] != -1) {
        data_next[tails[bucket]] = slot;
    } else {
        heads[bucket] = slot;
        bitmap_set(bucket);
    }
    tails[bucket] = slot;
    current_count++;
}

int pop_max() {
    int max_p = get_max_prio();
    if (max_p == -1) return -1;
    int slot = pop_head_internal(max_p);
    int id = data_ids[slot];
    free_slot(slot);
    return id;
}

// ==========================================
// 5. MAIN AVEC LOGIQUE DE SIMULATION
// ==========================================

int main() {
    init_system();
    srand(time(NULL));

    printf("=== SIMULATION POST INGESTION (Split @ 45) ===\n");
    printf("Capacite: %d | Total Posts: %d\n", CAPACITY, NUM_INSERTS);
    printf("Buckets: [0-25] Low Def | [26-255] High Def\n\n");

    clock_t start = clock();

    // PHASE 1: Injection de 60 Millions de posts
    // La structure va se remplir, puis commencer à éjecter les faibles risques
    for (int i = 0; i < NUM_INSERTS; i++) {
        // 1. Générer Float Risk
        double risk = generate_risk_score();
        
        // 2. Convertir en Bucket
        int bucket = risk_to_bucket_optimized(risk);
        
        // 3. Insérer (gère l'éviction auto)
        push(i, bucket);
        
        // Log progression
        if (i % 10000000 == 0 && i > 0) {
            int min_b = get_min_prio();
            printf("T=%dM posts | Min Bucket Stocke: %d\n", i/1000000, min_b);
        }
    }

    clock_t mid = clock();
    printf("\nFin ingestion. Stockage final (Top %d meilleurs):\n", current_count);
    printf("Min Bucket Final: %d (Doit etre > 25 pour valider la strategie)\n", get_min_prio());

    // PHASE 2: Vidage (Optionnel, juste pour tester la vitesse de sortie)
    /*
    long count = 0;
    while (1) {
        int id = pop_max();
        if (id == -1) break;
        count++;
    }
    */

    clock_t end = clock();
    double t_push = (double)(mid - start) / CLOCKS_PER_SEC;
    
    printf("\nTemps total ingestion + filtrage: %.4f sec\n", t_push);
    printf("Debit: %.2f M ops/sec\n", NUM_INSERTS / t_push / 1000000.0);

    return 0;
}