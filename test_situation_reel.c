#define _POSIX_C_SOURCE 200809L
#define _USE_MATH_DEFINES

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==========================================
// 1. CONFIGURATION DU SCÉNARIO "6 JOURS"
// ==========================================
#define CAPACITY        10000000  // Buffer de 10 Millions (Réaliste)
#define MAX_PRIO        255      
#define POSTS_PER_DAY   60000000  // 60 Millions de posts par jour
#define NUM_DAYS        6         // Simulation sur 6 jours
#define PROD_RATIO      10        // 10 posts produits...
#define CONS_RATIO      1         // ...pour 1 post consommé

#define CSV_FILENAME "data_6jours.csv"

// Calcul automatique du nombre de tours de boucle nécessaires
// Si on produit 10 posts par tour, il faut 6M tours pour faire 60M posts
#define STEPS_PER_DAY   (POSTS_PER_DAY / PROD_RATIO)
#define TOTAL_STEPS     (STEPS_PER_DAY * NUM_DAYS)

// ==========================================
// 2. MOTEUR (Optimisé pour la vitesse)
// ==========================================
int* data_ids;      
uint8_t* data_prios; 
int* data_next;     
uint64_t bitmap[4] = {0}; 
int heads[MAX_PRIO + 1]; 
int tails[MAX_PRIO + 1]; 
int bucket_counts[MAX_PRIO + 1] = {0}; // Compteur temps réel

int free_head = 0;    
int current_count = 0; 

// Outils Bitmap
static inline void bitmap_set(int p) { bitmap[p / 64] |= (1ULL << (p % 64)); }
static inline void bitmap_clear(int p) { bitmap[p / 64] &= ~(1ULL << (p % 64)); }

static inline int get_min_prio() {
    for (int i = 0; i < 4; i++) if (bitmap[i]) return (i * 64) + __builtin_ctzll(bitmap[i]);
    return -1;
}
static inline int get_max_prio() {
    for (int i = 3; i >= 0; i--) if (bitmap[i]) return (i * 64) + (63 - __builtin_clzll(bitmap[i]));
    return -1;
}

void init_system() {
    // Allocation de 10M d'entiers -> ~120 Mo de RAM, c'est léger.
    data_ids   = malloc(sizeof(int) * CAPACITY);
    data_prios = malloc(sizeof(uint8_t) * CAPACITY);
    data_next  = malloc(sizeof(int) * CAPACITY);
    
    if (!data_ids || !data_prios || !data_next) {
        printf("Erreur d'allocation mémoire (Trop gros ?)\n");
        exit(1);
    }

    for (int i = 0; i < CAPACITY - 1; i++) data_next[i] = i + 1;
    data_next[CAPACITY - 1] = -1;
    for (int i = 0; i <= MAX_PRIO; i++) { heads[i] = -1; tails[i] = -1; }
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
    heads[p] = next;
    if (heads[p] == -1) { tails[p] = -1; bitmap_clear(p); }
    
    current_count--;
    bucket_counts[p]--; 
    return slot;
}

void push(int id, int bucket) {
    if (current_count >= CAPACITY) {
        int min_bucket = get_min_prio();
        if (bucket <= min_bucket) return; // Rejet
        
        int dead = pop_head_internal(min_bucket);
        free_slot(dead);
    }
    
    int slot = alloc_slot();
    data_ids[slot] = id;
    data_prios[slot] = (uint8_t)bucket;
    data_next[slot] = -1;
    
    if (tails[bucket] != -1) data_next[tails[bucket]] = slot;
    else { heads[bucket] = slot; bitmap_set(bucket); }
    
    tails[bucket] = slot;
    current_count++;
    bucket_counts[bucket]++;
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
// 3. LOGIQUE MÉTIER & MAPPING
// ==========================================

double rand_normal(double mean, double stddev) {
    static double n2 = 0.0;
    static int n2_cached = 0;
    if (!n2_cached) {
        double u1 = (double)rand() / RAND_MAX;
        double u2 = (double)rand() / RAND_MAX;
        double x1 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
        n2 = sqrt(-2.0 * log(u1)) * sin(2.0 * M_PI * u2);
        n2_cached = 1;
        return mean + x1 * stddev;
    } else {
        n2_cached = 0;
        return mean + n2 * stddev;
    }
}

int risk_to_bucket(double risk) {
    if (risk < 0) risk = 0;
    if (risk > 100) risk = 100;
    
    // MAPPING OPTIMAL (Split 45)
    const double SPLIT = 45.0;
    const int THRESHOLD = (int)(255 * 0.10); 
    
    if (risk <= SPLIT) return (int)((risk / SPLIT) * THRESHOLD);
    return THRESHOLD + 1 + (int)((risk - SPLIT) / (100.0 - SPLIT) * (255 - THRESHOLD - 1));
}

// ==========================================
// 4. MAIN - SIMULATION JOUR PAR JOUR
// ==========================================
int main() {
    init_system();
    srand(42); 
    
    FILE* csv = fopen(CSV_FILENAME, "w");
    if (!csv) { perror("Erreur fichier"); return 1; }

    // En-tête CSV
    fprintf(csv, "Day,Total_Posts");
    for(int i=0; i<=MAX_PRIO; i++) fprintf(csv, ",B%d", i);
    fprintf(csv, "\n");

    printf("=== SIMULATION 6 JOURS ===\n");
    printf("Volume Total : %d Posts\n", POSTS_PER_DAY * NUM_DAYS);
    printf("Capacite RAM : %d\n", CAPACITY);
    printf("Ratio : %d Produits / %d Consommes\n\n", PROD_RATIO, CONS_RATIO);

    long long total_produced = 0;
    int day = 1;

    // --- ÉTAT INITIAL (T=0) ---
    fprintf(csv, "0,0");
    for (int b = 0; b <= MAX_PRIO; b++) fprintf(csv, ",0");
    fprintf(csv, "\n");

    // BOUCLE DE SIMULATION
    for (long long step = 1; step <= TOTAL_STEPS; step++) {
        
        // 1. PRODUCTION
        for (int p = 0; p < PROD_RATIO; p++) {
            double prob = (double)rand() / RAND_MAX;
            double risk;
            
            // Génération agressive (50% de risques élevés pour forcer le nettoyage)
            if (prob < 0.50) risk = fabs(rand_normal(0.0, 12.0)); 
            else if (prob < 0.80) risk = rand_normal(50.0, 10.0); 
            else risk = rand_normal(95.0, 5.0);                   
            
            push(total_produced++, risk_to_bucket(risk));
        }

        // 2. CONSOMMATION
        for (int c = 0; c < CONS_RATIO; c++) {
            pop_max();
        }

        // 3. FIN DE JOURNÉE (Snapshot)
        // On exporte si on a fini un bloc de "STEPS_PER_DAY"
        if (step % STEPS_PER_DAY == 0) {
            fprintf(csv, "%d,%lld", day, total_produced);
            
            for (int b = 0; b <= MAX_PRIO; b++) {
                fprintf(csv, ",%d", bucket_counts[b]);
            }
            fprintf(csv, "\n");
            
            printf("[JOUR %d TERMINÉ] %lld posts traités. Remplissage Buffer: %d/%d\n", 
                   day, total_produced, current_count, CAPACITY);
            day++;
        }
    }

    fclose(csv);
    printf("\nTerminé ! Ouvrez '%s' pour voir les résultats jour par jour.\n", CSV_FILENAME);
    return 0;
}