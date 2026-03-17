#!/bin/bash

# ================================
#  UNIT TESTS – PRIORITY SYSTEM
# ================================

GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

TESTS_PASSED=0
TESTS_FAILED=0
TEST_NUMBER=0
LINE_WIDTH=90

run_test() {
    local test_name="$1"
    local test_file="$2"

    TEST_NUMBER=$((TEST_NUMBER + 1))
    local label="Test ${TEST_NUMBER} - ${test_name}"
    local label_len=${#label}
    local dots_count=$((LINE_WIDTH - label_len - 4))
    local dots
    dots=$(printf '%*s' "$dots_count" '' | tr ' ' '.')

    printf "%s%s " "$label" "$dots"

    gcc -O3 -Wall -Wextra -std=c11 -o "$test_file" "${test_file}.c" priority_queue.o -lm 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL (compile)${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return
    fi

    "./$test_file" >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}OK${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}FAIL${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi

    rm -f "$test_file" "${test_file}.c"
}

echo "=========================================="
echo "  SUITE DE TESTS - SYSTÈME DE PRIORITÉ"
echo "=========================================="
echo ""

# Compile library (priority_queue.o)
printf "Compilation de la bibliothèque priority_queue.c..."
gcc -O3 -Wall -Wextra -std=c11 -c priority_queue.c -o priority_queue.o -lm 2>/dev/null
if [ $? -ne 0 ]; then
    echo -e " ${RED}FAIL${NC}"
    exit 1
fi
echo -e " ${GREEN}OK${NC}"
echo ""

########################
# TEST 1: init_system + get_max_prio
########################
cat > test1.c << 'EOF'
#include <stdint.h>
#include "priority_queue.h"

int main(void) {
    init_system();
    int maxp = get_max_prio();
    return (maxp == -1 && current_count == 0) ? 0 : 1;
}
EOF
run_test "isEmpty after initialization" "test1"

########################
# TEST 2: push + pop_max simple
########################
cat > test2.c << 'EOF'
#include "priority_queue.h"

int main(void) {
    init_system();
    push(1, 100);
    push(2, 200);
    int x = pop_max();  // should be id 2
    return (x == 2 && current_count == 1) ? 0 : 1;
}
EOF
run_test "pop_max returns highest bucket element" "test2"

########################
# TEST 3: pop_max on empty
########################
cat > test3.c << 'EOF'
#include "priority_queue.h"

int main(void) {
    init_system();
    int x = pop_max();
    return (x == -1 && current_count == 0) ? 0 : 1;
}
EOF
run_test "pop_max on empty queue returns -1" "test3"

########################
# TEST 4: multiple push and fifo order per bucket
########################
cat > test4.c << 'EOF'
#include "priority_queue.h"

int main(void) {
    init_system();
    // Push multiple items in same bucket
    push(10, 100);
    push(20, 100);
    push(30, 100);
    
    // Pop them - should come out in FIFO order
    int id1 = pop_max();
    int id2 = pop_max();
    int id3 = pop_max();
    
    // Check FIFO order within same bucket
    return (id1 == 10 && id2 == 20 && id3 == 30) ? 0 : 1;
}
EOF
run_test "FIFO order within same bucket" "test4"

########################
# TEST 5: priority ordering across buckets
########################
cat > test5.c << 'EOF'
#include "priority_queue.h"

int main(void) {
    init_system();
    push(1, 50);
    push(2, 150);
    push(3, 250);
    push(4, 100);
    push(5, 200);
    
    // Should pop in descending order: 250, 200, 150, 100, 50
    int prev_prio = 256;
    for (int i = 0; i < 5; i++) {
        int max_prio = get_max_prio();
        if (max_prio >= prev_prio) return 1;
        prev_prio = max_prio;
        pop_max();
    }
    return 0;
}
EOF
run_test "elements extracted by descending priority" "test5"

########################
# TEST 6: risk_to_bucket_optimized in range
########################
cat > test6.c << 'EOF'
#include "priority_queue.h"

int main(void) {
    for (int i = 0; i <= 100; ++i) {
        int b = risk_to_bucket_optimized((double)i);
        if (b < 0 || b > 255) return 1;
    }
    return 0;
}
EOF
run_test "risk_to_bucket_optimized stays in [0,255]" "test6"

########################
# TEST 7: generate_risk_score in [0,100]
########################
cat > test7.c << 'EOF'
#include <stdlib.h>
#include "priority_queue.h"

int main(void) {
    srand(42);
    for (int i = 0; i < 10000; ++i) {
        double r = generate_risk_score();
        if (r < 0.0 || r > 100.0) return 1;
    }
    return 0;
}
EOF
run_test "generate_risk_score stays in [0,100]" "test7"

########################
# TEST 8: get_min_prio works correctly
########################
cat > test8.c << 'EOF'
#include "priority_queue.h"

int main(void) {
    init_system();
    push(1, 50);
    push(2, 150);
    push(3, 250);
    
    int min = get_min_prio();
    int max = get_max_prio();
    
    return (min == 50 && max == 250) ? 0 : 1;
}
EOF
run_test "get_min_prio and get_max_prio work correctly" "test8"

########################
# TEST 9: bitmap operations
########################
cat > test9.c << 'EOF'
#include "priority_queue.h"

int main(void) {
    init_system();
    push(1, 0);    // min bucket
    push(2, 255);  // max bucket
    push(3, 128);  // middle
    
    int min = get_min_prio();
    int max = get_max_prio();
    
    pop_max();  // remove 255
    int new_max = get_max_prio();
    
    return (min == 0 && max == 255 && new_max == 128) ? 0 : 1;
}
EOF
run_test "bitmap correctly tracks bucket usage" "test9"

########################
# TEST 10: empty after full drain
########################
cat > test10.c << 'EOF'
#include "priority_queue.h"

int main(void) {
    init_system();
    push(1, 100);
    push(2, 200);
    push(3, 150);
    
    pop_max();
    pop_max();
    pop_max();
    
    int empty_check = pop_max();
    int max_prio = get_max_prio();
    
    return (empty_check == -1 && max_prio == -1 && current_count == 0) ? 0 : 1;
}
EOF
run_test "queue empty after draining all elements" "test10"

########################
# SUMMARY
########################
echo ""
echo "=========================================="
echo "         RÉSUMÉ DES TESTS"
echo "=========================================="
TOTAL=$((TESTS_PASSED + TESTS_FAILED))
echo -e "Total de tests: ${TOTAL}"
echo -e "${GREEN}Tests réussis: ${TESTS_PASSED}${NC}"
echo -e "${RED}Tests échoués: ${TESTS_FAILED}${NC}"

rm -f priority_queue.o

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "\n${GREEN}✓ TOUS LES TESTS SONT PASSÉS !${NC}"
    exit 0
else
    echo -e "\n${RED}✗ CERTAINS TESTS ONT ÉCHOUÉ${NC}"
    exit 1
fi
