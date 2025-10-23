#include "tabletranspo.h"
#include <string.h>
#include <omp.h>
static omp_lock_t transpo_lock_J1;
static omp_lock_t transpo_lock_J2;


Entry transpo_table_J1[HASH_SIZE];
Entry transpo_table_J2[HASH_SIZE];


void initialiser_transpo_lock() {
    omp_init_lock(&transpo_lock_J1);
    omp_init_lock(&transpo_lock_J2);

}



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

int chercher_transpo(int joueur, unsigned long long key, int profondeur, int* score_out) {
    Entry* table = (joueur == 1) ? transpo_table_J1 : transpo_table_J2;
    unsigned int idx = key % HASH_SIZE;
    int found = 0;
    if (table[idx].valid &&
        table[idx].key == key &&
        table[idx].depth >= profondeur) {
        *score_out = table[idx].score;
        found = 1;
    }
    return found;
}


void ajouter_transpo(int joueur, unsigned long long key, int profondeur, int score) {
    Entry* table = (joueur == 1) ? transpo_table_J1 : transpo_table_J2;
    unsigned int idx = key % HASH_SIZE;
    omp_lock_t* lock = (joueur == 1) ? &transpo_lock_J1 : &transpo_lock_J2;
    omp_set_lock(lock);   // 🔒 début section protégée
    table[idx].key = key;
    table[idx].depth = profondeur;
    table[idx].score = score;
    table[idx].valid = 1;
    omp_unset_lock(lock); // 🔓 fin section protégée
}


void vider_transpo_joueur(int joueur) {
    if (joueur == 1)
        memset(transpo_table_J1, 0, sizeof(transpo_table_J1));
    else
        memset(transpo_table_J2, 0, sizeof(transpo_table_J2));
}

void vider_toutes_transpos() {
    memset(transpo_table_J1, 0, sizeof(transpo_table_J1));
    memset(transpo_table_J2, 0, sizeof(transpo_table_J2));
}
