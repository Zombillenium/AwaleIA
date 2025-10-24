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
            if (total == 2 || total == 3)
                casesCapturablesJ++;
        } else {
            totalA += total;
            transA += p->T;
            if (total == 2 || total == 3)
                casesCapturablesA++;
        }
        p = p->caseSuiv;
    } while (p != plateau);

    // pondération simple
    float score = 0.0f;
    score += 3.0f * totalJ;                            // graines totales du joueur
    score -= 2.0f * totalA;                            // graines de l’adversaire
    score += 5.0f * (casesCapturablesJ - casesCapturablesA);
    score += 1.0f * (transJ - transA);

    // mise à l’échelle dans [1, 1000]
    if (score < -200.0f) score = -200.0f;
    if (score > 200.0f)  score = 200.0f;

    int eval = (int)(500.0f + (score * 2.5f));         // 0 → 500, +200 → 1000, -200 → 0
    if (eval < 1) eval = 1;
    if (eval > 1000) eval = 1000;

    return eval;
}


int evaluer_plateau_famine(Plateau* plateau, int joueur) {
    if (!plateau) return 500;

    int grainesJ = 0, grainesA = 0;
    int casesVidesJ = 0, casesVidesA = 0;
    int coupsJ = 0, coupsA = 0;
    int captJ = 0, captA = 0;
    int famine1J = 0, famine1A = 0;

    Plateau* p = plateau;
    do {
        int total = p->R + p->B + p->T;
        if (case_du_joueur(p->caseN, joueur)) {
            grainesJ += total;
            if (total == 0) casesVidesJ++;
            if (total > 0) coupsJ++;
            if (total == 2 || total == 3) captJ++;
            if (total == 1) famine1J++;
        } else {
            grainesA += total;
            if (total == 0) casesVidesA++;
            if (total > 0) coupsA++;
            if (total == 2 || total == 3) captA++;
            if (total == 1) famine1A++;
        }
        p = p->caseSuiv;
    } while (p != plateau);

    float total = grainesJ + grainesA;
    float phase = 1.0f - (total / 48.0f);  // 0 = début, 1 = fin

    float score = 0.0f;

    // === noyau famine ===
    // plus de vide et de cases à 1 graine chez l’adversaire = bonne pression
    score += 40.0f * (casesVidesA - casesVidesJ);
    score += 25.0f * (famine1A - famine1J);

    // === mobilité et survie ===
    score += 10.0f * (coupsJ - coupsA);
    score += 10.0f * (captJ - captA);

    // === ressources ===
    score += 6.0f * (grainesJ - grainesA);

    // === pression de phase : fin de partie plus tranchée ===
    score += phase * 20.0f * ((casesVidesA + famine1A) - (casesVidesJ + famine1J));

    // === bonus de situation ===
    if (grainesA <= 6) score += (30 - 3 * grainesA);
    if (coupsA <= 2)   score += 25;
    if (casesVidesA >= 6) score += 20;

    // === malus survie personnelle ===
    if (grainesJ <= 6) score -= (30 - 3 * grainesJ);
    if (coupsJ <= 2)   score -= 25;
    if (casesVidesJ >= 6) score -= 20;

    // si on domine clairement, passer en mode agressif
    if (grainesA < grainesJ - 6) {
        score += 15.0f * (captJ - captA);
        score += 8.0f  * (coupsJ - coupsA);
    }

    // si on mène au score total, accepter plus de risque
    if (grainesJ > grainesA + 8) {
        score += 0.5f * (grainesJ - grainesA);
    }
    

    // === stabilisation : éviter saturation ===
    if (score < -250) score = -250;
    if (score > 250)  score = 250;

    int eval = (int)(500 + (score * 2.0f));  // plage [0–1000]
    if (eval < 1) eval = 1;
    if (eval > 1000) eval = 1000;
    
    return eval;
}
