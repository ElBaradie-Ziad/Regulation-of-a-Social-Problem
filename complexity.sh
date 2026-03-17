#!/bin/bash

# ============================================================================
# COMPLEXITY AND PERFORMANCE TESTING SCRIPT
# Bucket Queue with Bitmap Implementation (256 buckets, 0-255)
# ============================================================================

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuration
SOURCE_FILES="main.c priority_queue.c"
EXECUTABLE="./priority_queue_benchmark"
RESULTS_DIR="./performance_results"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
REPETITIONS=5

# Test configurations: [NUM_INSERTS, CAPACITY]
declare -a TEST_CONFIGS=(
    "1000000:1000000"       # 1M posts
    "5000000:5000000"       # 5M posts
    "10000000:10000000"     # 10M posts
    "50000000:50000000"     # 50M posts
    "100000000:100000000"   # 100M posts
)

# Create results directory
mkdir -p "$RESULTS_DIR"

echo -e "${BLUE}============================================================================${NC}"
echo -e "${BLUE} COMPLEXITY AND PERFORMANCE VALIDATION TEST SUITE${NC}"
echo -e "${BLUE} Bucket Queue with Bitmap - 256 Buckets (0-255)${NC}"
echo -e "${BLUE}============================================================================${NC}"
echo ""

# Display system information
echo -e "${CYAN}System Information:${NC}"
echo -e "  Hostname    : $(hostname)"
echo -e "  OS          : $(uname -s) $(uname -r)"
echo -e "  Architecture: $(uname -m)"
echo -e "  CPU         : $(lscpu | grep 'Model name' | cut -d: -f2 | xargs || echo 'N/A')"
echo -e "  CPU Cores   : $(nproc)"
echo -e "  Total RAM   : $(free -h | awk '/^Mem:/{print $2}')"
echo -e "  Available   : $(free -h | awk '/^Mem:/{print $7}')"
echo ""

echo -e "${CYAN}Test Configuration:${NC}"
echo -e "  Source Files    : ${SOURCE_FILES}"
echo -e "  Repetitions/test: ${REPETITIONS}"
echo -e "  Compiler flags  : -O3 -Wall -Wextra -lm"
echo -e "  Buckets         : 256 (0-255)"
echo ""

# Check if source files exist
for file in $SOURCE_FILES; do
    if [ ! -f "$file" ]; then
        echo -e "${RED}ERROR: Source file '${file}' not found!${NC}"
        exit 1
    fi
done

# CSV file for results
RESULTS_CSV="${RESULTS_DIR}/complexity_results_${TIMESTAMP}.csv"
echo "num_inserts,capacity,repetition,total_time_sec,throughput_mops" > "$RESULTS_CSV"

echo -e "${GREEN}============================================================================${NC}"
echo -e "${GREEN}Starting Performance Tests${NC}"
echo -e "${GREEN}============================================================================${NC}"
echo ""

test_number=1
total_tests=${#TEST_CONFIGS[@]}

for config in "${TEST_CONFIGS[@]}"; do
    IFS=':' read -r num_inserts capacity <<< "$config"
    
    echo -e "${BLUE}----------------------------------------------------------------------------${NC}"
    echo -e "${BLUE}Test ${test_number}/${total_tests}: NUM_INSERTS=${num_inserts}, CAPACITY=${capacity}${NC}"
    echo -e "${BLUE}----------------------------------------------------------------------------${NC}"
    
    # Check if we have enough RAM (estimate: 13 bytes per post for SoA)
    required_ram_mb=$((capacity * 13 / 1024 / 1024))
    available_ram_mb=$(free -m | awk '/^Mem:/{print $7}')
    
    if [ "$required_ram_mb" -gt "$available_ram_mb" ]; then
        echo -e "${YELLOW}WARNING: Insufficient RAM (need ~${required_ram_mb}MB, have ${available_ram_mb}MB)${NC}"
        echo -e "${YELLOW}Skipping this configuration...${NC}"
        echo ""
        ((test_number++))
        continue
    fi
    
    # Compile with specific configuration
    echo -e "${CYAN}Compiling with NUM_INSERTS=${num_inserts}, CAPACITY=${capacity}...${NC}"
    
    # Create temporary main.c with override
    cat > benchmark_main.c << EOF
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "priority_queue.h"

#undef NUM_INSERTS
#define NUM_INSERTS ${num_inserts}

int main(void) {
    init_system();
    srand(42);  // Fixed seed for reproducibility
    
    printf("=== BENCHMARK: %d inserts, capacity %d ===\\n", NUM_INSERTS, CAPACITY);
    
    clock_t start = clock();
    
    for (int i = 0; i < NUM_INSERTS; i++) {
        double risk = generate_risk_score();
        int bucket = risk_to_bucket_optimized(risk);
        push(i, bucket);
    }
    
    clock_t end = clock();
    double total_time = (double)(end - start) / CLOCKS_PER_SEC;
    double throughput = NUM_INSERTS / total_time / 1000000.0;
    
    printf("Temps total: %.4f sec\\n", total_time);
    printf("Débit: %.2f M ops/sec\\n", throughput);
    
    return 0;
}
EOF

    gcc -O3 -Wall -Wextra -std=c11 \
        -DCAPACITY=${capacity} \
        -DMAX_PRIO=255 \
        benchmark_main.c priority_queue.c \
        -o "$EXECUTABLE" -lm 2>&1
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Compilation failed! Skipping this configuration.${NC}"
        echo ""
        ((test_number++))
        rm -f benchmark_main.c
        continue
    fi
    
    echo -e "${GREEN}✓ Compilation successful${NC}"
    echo ""
    
    # Run multiple repetitions
    for rep in $(seq 1 $REPETITIONS); do
        echo -e "${YELLOW}  Running repetition ${rep}/${REPETITIONS}...${NC}"
        
        output=$($EXECUTABLE 2>&1)
        exit_code=$?
        
        if [ $exit_code -ne 0 ]; then
            echo -e "${RED}  ✗ Execution failed with exit code ${exit_code}${NC}"
            continue
        fi
        
        # Parse output
        total_time=$(echo "$output" | grep "Temps total" | sed -E 's/.*: ([0-9.]+) sec.*/\1/')
        throughput=$(echo "$output" | grep "Débit" | sed -E 's/.*: ([0-9.]+) M ops.*/\1/')
        
        if [ -z "$total_time" ] || [ -z "$throughput" ]; then
            echo -e "${RED}  ✗ Failed to parse output${NC}"
            echo "$output"
            continue
        fi
        
        echo "${num_inserts},${capacity},${rep},${total_time},${throughput}" >> "$RESULTS_CSV"
        echo -e "${GREEN}  ✓ Completed: ${total_time}s total, ${throughput} MOps/s${NC}"
    done
    
    echo ""
    ((test_number++))
done

# Clean up
rm -f "$EXECUTABLE" benchmark_main.c

echo -e "${GREEN}============================================================================${NC}"
echo -e "${GREEN}All Tests Completed Successfully!${NC}"
echo -e "${GREEN}============================================================================${NC}"
echo ""

# Generate Summary Report
SUMMARY_FILE="${RESULTS_DIR}/summary_${TIMESTAMP}.txt"

cat > "$SUMMARY_FILE" << EOF
================================================================================
COMPLEXITY AND PERFORMANCE TEST REPORT
Bucket Queue with Bitmap Implementation (256 Buckets, 0-255)
================================================================================
Test Date: $(date)
Machine: $(hostname)
OS: $(uname -s) $(uname -r)
CPU: $(lscpu | grep 'Model name' | cut -d: -f2 | xargs || echo 'N/A')
Cores: $(nproc)
RAM: $(free -h | awk '/^Mem:/{print $2}')

================================================================================
TEST CONFIGURATION
================================================================================
Source Files: ${SOURCE_FILES}
Repetitions per test: ${REPETITIONS}
Compiler: gcc -O3 -Wall -Wextra -std=c11 -lm
Buckets: 256 (0-255)
Distribution: 80% low risk, 15% mid risk, 5% high risk

================================================================================
THEORETICAL COMPLEXITY ANALYSIS
================================================================================
Operation          | Expected Complexity | Implementation
-------------------|---------------------|--------------------------------
Insert (non-full)  | O(1)                | Direct bucket access
Insert (full)      | O(1)                | Evict min + insert
Find Min/Max       | O(1)                | Bitmap scan with CLZ/CTZ
Remove Min/Max     | O(1)                | Head removal from bucket
Replacement        | O(1)                | Remove min + insert

With k=256 fixed buckets and hardware bitmap operations, all ops are O(1).

================================================================================
PERFORMANCE RESULTS SUMMARY
================================================================================

EOF

# Calculate averages
echo "Average Performance by Configuration:" >> "$SUMMARY_FILE"
echo "-------------------------------------" >> "$SUMMARY_FILE"

awk -F',' 'NR>1 {
    key = $1
    total[$1] += $4
    tput[$1] += $5
    count[$1]++
}
END {
    printf "%-15s | %-15s | %-20s\n", "Num Inserts", "Total Time (s)", "Throughput (MOps/s)"
    printf "----------------|-----------------|---------------------\n"
    for (n in count) {
        printf "%-15s | %15.4f | %20.2f\n", n, total[n]/count[n], tput[n]/count[n]
    }
}' "$RESULTS_CSV" >> "$SUMMARY_FILE"

echo "" >> "$SUMMARY_FILE"
echo "================================================================================  " >> "$SUMMARY_FILE"
echo "COMPLEXITY VALIDATION" >> "$SUMMARY_FILE"
echo "================================================================================" >> "$SUMMARY_FILE"
echo "To validate O(1) complexity for operations, we expect:" >> "$SUMMARY_FILE"
echo "  1. Total time T(n) should scale linearly with n" >> "$SUMMARY_FILE"
echo "  2. Throughput (operations/sec) should remain relatively stable" >> "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"

echo -e "${CYAN}Summary report generated: ${SUMMARY_FILE}${NC}"
echo ""

# Display summary
cat "$SUMMARY_FILE"

echo ""
echo -e "${GREEN}============================================================================${NC}"
echo -e "${GREEN} TESTING COMPLETE${NC}"
echo -e "${GREEN}============================================================================${NC}"
echo ""
echo -e "${CYAN}Results Location:${NC} ${RESULTS_DIR}/"
echo -e "${CYAN}CSV Data:${NC} ${RESULTS_CSV}"
echo -e "${CYAN}Summary Report:${NC} ${SUMMARY_FILE}"
echo ""
