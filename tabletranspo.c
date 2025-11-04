#include "tabletranspo.h"
#include <string.h>
#include <omp.h>
static omp_lock_t transpo_lock_J1;
static omp_lock_t transpo_lock_J2;


static unsigned long long zobrist[MAX_CASES][3][MAX_GRAINES + 1];

Entry transpo_table_J1[HASH_SIZE];
Entry transpo_table_J2[HASH_SIZE];


void initialiser_transpo_lock() {
    omp_init_lock(&transpo_lock_J1);
    omp_init_lock(&transpo_lock_J2);

}

void initialiser_zobrist() {
    unsigned long long seed = 88172645463325252ULL; // arbitraire
    for (int i = 0; i < MAX_CASES; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k <= MAX_GRAINES; k++) {
                // Xorshift simple
                seed ^= seed << 13;
                seed ^= seed >> 7;
                seed ^= seed << 17;
                zobrist[i][j][k] = seed;
            }
        }
    }
}



unsigned long long hash_plateau(Plateau* plateau, int nb_cases) {
    unsigned long long h = 0ULL;
    Plateau* p = plateau;
    for (int i = 0; i < nb_cases; i++) {
        int r = (p->R <= MAX_GRAINES) ? p->R : MAX_GRAINES;
        int b = (p->B <= MAX_GRAINES) ? p->B : MAX_GRAINES;
        int t = (p->T <= MAX_GRAINES) ? p->T : MAX_GRAINES;

        h ^= zobrist[i][0][r];
        h ^= zobrist[i][1][b];
        h ^= zobrist[i][2][t];

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
