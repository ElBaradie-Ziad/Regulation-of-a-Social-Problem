#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "priority_queue.h"

#define NUM_INSERTS 90000000

int main(void) {
    init_system();
    srand(time(NULL));
    
    printf("=== SIMULATION POST INGESTION (Split @ 45) ===\n");
    printf("Capacite: %d | Total Posts: %d\n", CAPACITY, NUM_INSERTS);
    printf("Buckets: [0-25] Low Def | [26-255] High Def\n\n");
    
    clock_t start = clock();
    
    for (int i = 0; i < NUM_INSERTS; i++) {
        double risk = generate_risk_score();
        int bucket = risk_to_bucket_optimized(risk);
        push(i, bucket);
        
        if (i % 10000000 == 0 && i > 0) {
            int min_b = get_min_prio();
            printf("T=%dM posts | Min Bucket Stocke: %d\n", i/1000000, min_b);
        }
    }
    
    clock_t mid = clock();
    printf("\nFin ingestion. Stockage final (Top %d meilleurs):\n", current_count);
    printf("Min Bucket Final: %d (Doit etre > 25 pour valider la strategie)\n", get_min_prio());
    
    double t_push = (double)(mid - start) / CLOCKS_PER_SEC;
    printf("\nTemps total ingestion + filtrage: %.4f sec\n", t_push);
    printf("Debit: %.2f M ops/sec\n", NUM_INSERTS / t_push / 1000000.0);
    
    return 0;
}
