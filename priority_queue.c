#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "priority_queue.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Variables globales
int* data_ids;
uint8_t* data_prios;
int* data_next;
uint64_t bitmap[4] = {0};
int heads[MAX_PRIO + 1];
int tails[MAX_PRIO + 1];
int free_head = 0;
int current_count = 0;

// ==========================================
// Fonctions bitmap
// ==========================================
static inline void bitmap_set(int p) {
    bitmap[p / 64] |= (1ULL << (p % 64));
}

static inline void bitmap_clear(int p) {
    bitmap[p / 64] &= ~(1ULL << (p % 64));
}

int get_max_prio(void) {
    for (int i = 3; i >= 0; i--) {
        if (bitmap[i]) {
            return (i * 64) + (63 - __builtin_clzll(bitmap[i]));
        }
    }
    return -1;
}

int get_min_prio(void) {
    for (int i = 0; i < 4; i++) {
        if (bitmap[i]) {
            return (i * 64) + __builtin_ctzll(bitmap[i]);
        }
    }
    return -1;
}

// ==========================================
// Génération aléatoire
// ==========================================
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

double generate_risk_score(void) {
    double p = (double)rand() / RAND_MAX;
    double risk;
    if (p < 0.80) {
        risk = fabs(rand_normal(0.0, 12.0));
    } else if (p < 0.95) {
        risk = rand_normal(50.0, 8.0);
    } else {
        risk = rand_normal(92.0, 6.0);
    }
    if (risk < 0.0) return 0.0;
    if (risk > 100.0) return 100.0;
    return risk;
}

int risk_to_bucket_optimized(double risk) {
    const double SPLIT_RISK = 45.0;
    const int N_BUCKETS = 255;
    const double RATIO_FOCUS = 0.90;
    int bucket_threshold = (int)(N_BUCKETS * (1.0 - RATIO_FOCUS));
    if (risk <= SPLIT_RISK) {
        return (int)((risk / SPLIT_RISK) * bucket_threshold);
    } else {
        double range_risk = 100.0 - SPLIT_RISK;
        double range_bucket = N_BUCKETS - bucket_threshold;
        int b = bucket_threshold + 1 + (int)((risk - SPLIT_RISK) / range_risk * (range_bucket - 1));
        if (b > 255) return 255;
        return b;
    }
}

// ==========================================
// Gestion mémoire
// ==========================================
void init_system(void) {
    data_ids = malloc(sizeof(int) * CAPACITY);
    data_prios = malloc(sizeof(uint8_t) * CAPACITY);
    data_next = malloc(sizeof(int) * CAPACITY);
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

static inline int alloc_slot(void) {
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

// ==========================================
// Opérations publiques
// ==========================================
void push(int id, int bucket) {
    if (current_count >= CAPACITY) {
        int min_bucket = get_min_prio();
        if (bucket <= min_bucket) return;
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

int pop_max(void) {
    int max_p = get_max_prio();
    if (max_p == -1) return -1;
    int slot = pop_head_internal(max_p);
    int id = data_ids[slot];
    free_slot(slot);
    return id;
}
