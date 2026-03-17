#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stdint.h>

// Configuration
#ifndef CAPACITY
#define CAPACITY 10000000  
#endif


#ifndef MAX_PRIO
#define MAX_PRIO 255
#endif


// Variables globales exportées
extern int* data_ids;
extern uint8_t* data_prios;
extern int* data_next;
extern uint64_t bitmap[4];
extern int heads[MAX_PRIO + 1];
extern int tails[MAX_PRIO + 1];
extern int free_head;
extern int current_count;

// Fonctions publiques
void init_system(void);
void push(int id, int bucket);
int pop_max(void);
int get_max_prio(void);
int get_min_prio(void);
double rand_normal(double mean, double stddev);
double generate_risk_score(void);
int risk_to_bucket_optimized(double risk);

#endif
