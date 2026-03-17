CC = gcc
CFLAGS = -O3 -Wall -Wextra -std=c11
LDFLAGS = -lm

TARGET = main
OBJS = main.o priority_queue.o

# Cibles supplémentaires
REAL_TARGET = test_situation_reel
REAL_OBJS = test_situation_reel.o

GREEN = \033[0;32m
RED = \033[0;31m
BLUE = \033[0;34m
CYAN = \033[0;36m
YELLOW = \033[1;33m
NC = \033[0m

all: $(TARGET)
	@echo "$(GREEN)✓ Compilation terminée avec succès$(NC)"

$(TARGET): $(OBJS)
	@echo "$(BLUE)Linking $(TARGET)...$(NC)"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilation du test situation réelle (avec pthread ET math lib)
$(REAL_TARGET): $(REAL_OBJS)
	@echo "$(BLUE)Linking $(REAL_TARGET)...$(NC)"
	$(CC) $(CFLAGS) -o $@ $^ -lm -lpthread

# Règle pour compiler test_situation_reel.o avec defines POSIX
test_situation_reel.o: test_situation_reel.c
	@echo "$(BLUE)Compiling test_situation_reel.c...$(NC)"
	$(CC) $(CFLAGS) -D_POSIX_C_SOURCE=200809L -c test_situation_reel.c -o test_situation_reel.o

%.o: %.c
	@echo "$(BLUE)Compiling $<...$(NC)"
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	@echo "$(BLUE)Exécution du programme...$(NC)"
	./$(TARGET)

real: $(REAL_TARGET)
	@echo "$(CYAN)========================================$(NC)"
	@echo "$(CYAN)Simulation en situation réelle$(NC)"
	@echo "$(CYAN)Durée: 30 secondes$(NC)"
	@echo "$(CYAN)========================================$(NC)"
	./$(REAL_TARGET)
	@echo ""
	@echo "$(GREEN)✓ Simulation terminée$(NC)"
	@echo "$(GREEN)✓ Fichier CSV généré: data_6jours.csv$(NC)"
	@echo ""
	@echo "$(YELLOW)Génération des graphiques...$(NC)"
	@if command -v python3 >/dev/null 2>&1; then \
		python3 plot_buckets.py; \
		echo "$(GREEN)✓ Graphiques générés dans graphiques_output/$(NC)"; \
	else \
		echo "$(RED)✗ Python3 non trouvé. Installez-le pour générer les graphiques.$(NC)"; \
	fi

test: priority_queue.o
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Lancement de la suite de tests$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@chmod +x test.sh
	@./test.sh

comp: $(TARGET)
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Analyse de complexité$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@chmod +x complexity.sh
	@./complexity.sh


env:
	@echo "$(CYAN)Création de l'environnement virtuel Python (.venv) et installation des requirements...$(NC)"
	@if [ -d .venv ]; then \
		echo ".venv déjà présent"; \
	else \
		python3 -m venv .venv; \
	fi
	@# Activation cross-platform et installation requirements
	@. .venv/bin/activate 2>/dev/null || . .venv/Scripts/activate; \
	pip install --upgrade pip setuptools wheel; \
	pip install -r requirements.txt

help:
	@echo "$(CYAN)Makefile - File de Priorité$(NC)"
	@echo ""
	@echo "Cibles disponibles:"
	@echo "  $(GREEN)make$(NC) / $(GREEN)make all$(NC)    - Compile le programme principal"
	@echo "  $(GREEN)make run$(NC)            - Exécute le programme principal"
	@echo "  $(GREEN)make real$(NC)           - Lance simulation réelle (30s) + graphiques"
	@echo "  $(GREEN)make test$(NC)           - Lance la suite de tests unitaires"
	@echo "  $(GREEN)make comp$(NC)           - Lance l'analyse de complexité"
	@echo "  $(GREEN)make env$(NC)            - Crée l'env Python et installe requirements.txt"
	@echo "  $(GREEN)make clean$(NC)          - Nettoie tous les fichiers générés"
	@echo "  $(GREEN)make re$(NC)             - Recompile complètement"
	@echo "  $(GREEN)make valgrind$(NC)       - Exécute le programme principal sous Valgrind pour vérifier les fuites de mémoire"
	@echo "  $(GREEN)make valgrind-real$(NC)  - Exécute test_situation_reel sous Valgrind pour vérifier les fuites de mémoire"
	@echo "  $(GREEN)make help$(NC)           - Affiche cette aide"

.PHONY: valgrind

valgrind: $(TARGET)
	@echo "$(YELLOW)Exécution de Valgrind sur $(TARGET)...$(NC)"
	valgrind --leak-check=full --track-origins=yes ./$(TARGET)
.PHONY: all run real test comp clean re help env

.PHONY: valgrind-real

valgrind-real: $(REAL_TARGET)
	@echo "$(YELLOW)Exécution de Valgrind sur $(REAL_TARGET)...$(NC)"
	valgrind --leak-check=full --track-origins=yes ./$(REAL_TARGET)


clean:
	@echo "$(RED)Nettoyage...$(NC)"
	rm -f $(OBJS) $(REAL_OBJS) $(TARGET) $(REAL_TARGET)
	rm -f test_empty test_delete test_delete_empty test_add_empty test_add_full
	rm -f test_add_full_low test_bitmap test_risk_range test_sorted test_replace
	rm -f test[0-9] test[0-9][0-9]
	rm -f benchmark_main.c priority_queue_benchmark
	rm -f data_buckets.csv data_6jours.csv
	rm -rf performance_results graphiques_output

re: clean all	
