#include "evaluation.h"
#include "plateau.h"  

int evaluer_plateau(Plateau* plateau, int joueur) {
    int totalJ = 0, totalA = 0;
    int transJ = 0, transA = 0;
    int casesCapturablesJ = 0, casesCapturablesA = 0;

    Plateau* p = plateau;
    do {
        int total = p->R + p->B + p->T;
        if (case_du_joueur(p->caseN, joueur)) {
            totalJ += total;
            transJ += p->T;
            if (total == 2 || total == 3) casesCapturablesJ++;
        } else {
            totalA += total;
            transA += p->T;
            if (total == 2 || total == 3) casesCapturablesA++;
        }
        p = p->caseSuiv;
    } while (p != plateau);

    // pondération simple
    float score = 0;
    score += 3.0 * totalJ;               // graines totales du joueur
    score -= 2.0 * totalA;               // graines de l’adversaire
    score += 5.0 * (casesCapturablesJ - casesCapturablesA);
    score += 1.0 * (transJ - transA);

    // mise à l’échelle dans [1, 1000]
    if (score < -200) score = -200;
    if (score > 200)  score = 200;
    int eval = (int)(500 + (score * 2.5)); // 0 → 500, +200 → 1000, -200 → 0
    if (eval < 1) eval = 1;
    if (eval > 1000) eval = 1000;

    return eval;
}


int evaluer_plateau_famine(Plateau* plateau, int joueur) {
    if (!plateau) return 500;

    int grainesJ = 0, grainesA = 0;
    int casesVidesJ = 0, casesVidesA = 0;
    int coupsPossiblesJ = 0, coupsPossiblesA = 0;
    int capturesPotentiellesJ = 0, capturesPotentiellesA = 0;
    int faminePotA = 0, faminePotJ = 0;

    Plateau* p = plateau;
    do {
        int total = p->R + p->B + p->T;
        if (case_du_joueur(p->caseN, joueur)) {
            grainesJ += total;
            if (total == 0) casesVidesJ++;
            if (total > 0) coupsPossiblesJ++;
            if (total == 2 || total == 3) capturesPotentiellesJ++;
            if (total == 1) faminePotJ++;
        } else {
            grainesA += total;
            if (total == 0) casesVidesA++;
            if (total > 0) coupsPossiblesA++;
            if (total == 2 || total == 3) capturesPotentiellesA++;
            if (total == 1) faminePotA++;
        }
        p = p->caseSuiv;
    } while (p != plateau);

    // Score heuristique famine
    float score = 0.0f;

    // Objectif principal : pousser l’adversaire vers la famine
    score += 50.0f * (casesVidesA - casesVidesJ);
    score += 20.0f * (faminePotA - faminePotJ);
    score += 15.0f * (capturesPotentiellesJ - capturesPotentiellesA);
    score += 10.0f * (coupsPossiblesJ - coupsPossiblesA);
    score += 8.0f * (grainesJ - grainesA);

    // Bonus si famine imminente de l’adversaire
    if (grainesA <= 6) score += (50 - 5 * grainesA);
    if (coupsPossiblesA <= 2) score += 30;
    if (casesVidesA >= 6) score += 25;

    // Malus si nous sommes proches de la famine
    if (grainesJ <= 6) score -= (50 - 5 * grainesJ);
    if (coupsPossiblesJ <= 2) score -= 20;
    if (casesVidesJ >= 6) score -= 25;

    // Normalisation
    if (score < -200) score = -200;
    if (score > 200)  score = 200;

    int eval = (int)(500 + (score * 2.5f));
    if (eval < 1) eval = 1;
    if (eval > 1000) eval = 1000;

    return eval;
}
