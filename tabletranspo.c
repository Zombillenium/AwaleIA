#include "tabletranspo.h"
#include <string.h>

Entry transpo_table[HASH_SIZE];

unsigned long long hash_plateau(Plateau* plateau, int nb_cases) {
    unsigned long long h = 1469598103934665603ULL; // FNV offset basis
    Plateau* p = plateau;
    for (int i = 0; i < nb_cases; i++) {
        // Mélange les valeurs des graines et du numéro de case
        unsigned long long val = (p->R * 131 + p->B * 137 + p->T * 139 + p->caseN);
        h ^= val;
        h *= 1099511628211ULL; // FNV prime
        p = p->caseSuiv;
    }
    return h;
}

int chercher_transpo(unsigned long long key, int profondeur, int* score_out) {
    unsigned int idx = key % HASH_SIZE;
    if (transpo_table[idx].valid &&
        transpo_table[idx].key == key &&
        transpo_table[idx].depth >= profondeur) {
        *score_out = transpo_table[idx].score;
        return 1; // trouvé
    }
    return 0;
}

void ajouter_transpo(unsigned long long key, int profondeur, int score) {
    unsigned int idx = key % HASH_SIZE;
    transpo_table[idx].key = key;
    transpo_table[idx].depth = profondeur;
    transpo_table[idx].score = score;
    transpo_table[idx].valid = 1;
}

void vider_transpo() {
    memset(transpo_table, 0, sizeof(transpo_table));
}
