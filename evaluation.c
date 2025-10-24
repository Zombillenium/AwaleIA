#include "evaluation.h"
#include "plateau.h"
#include <stdlib.h> // pour rand()

// ===============================
// IA CLASSIQUE (équilibrée)
// ===============================
int evaluer_plateau(Plateau* plateau, int joueur) {
    int totalJ = 0, totalA = 0;
    int transJ = 0, transA = 0;
    int captJ = 0, captA = 0;
    int coupsJ = 0, coupsA = 0;
    int casesVidesJ = 0, casesVidesA = 0;

    Plateau* p = plateau;
    do {
        int total = p->R + p->B + p->T;
        if (case_du_joueur(p->caseN, joueur)) {
            totalJ += total;
            transJ += p->T;
            if (total > 0) coupsJ++;
            if (total == 0) casesVidesJ++;
            if (total == 2 || total == 3) captJ++;
        } else {
            totalA += total;
            transA += p->T;
            if (total > 0) coupsA++;
            if (total == 0) casesVidesA++;
            if (total == 2 || total == 3) captA++;
        }
        p = p->caseSuiv;
    } while (p != plateau);

    // Phase de jeu : 0 = début, 1 = fin
    float phase = 1.0f - ((float)(totalJ + totalA) / 48.0f);
    float score = 0.0f;

    // === stratégie principale ===
    score += (3.0f + 2.0f * phase) * (totalJ - totalA);         // avantage en graines
    score += (5.0f + 3.0f * phase) * (captJ - captA);           // captures
    score += 1.0f * (transJ - transA);                          // transitions
    score += 3.0f * (coupsJ - coupsA);                          // mobilité
    score -= 2.0f * (casesVidesJ - casesVidesA);                // vides = instabilité

    // === sécurité personnelle ===
    if (totalJ <= 6) score -= (30 - 3 * totalJ);
    if (coupsJ <= 2) score -= 25;
    if (casesVidesJ >= 6) score -= 20;

    // === pression sur l’adversaire ===
    if (totalA <= 6) score += (30 - 3 * totalA);
    if (coupsA <= 2) score += 25;
    if (casesVidesA >= 6) score += 20;

    // === bonus d’équilibre global (fin de partie) ===
    if ((totalJ + totalA) < 30) score += 5.0f;

    // === stabilisation et bruit léger ===
    if (score < -200) score = -200;
    if (score > 200)  score = 200;

    static int last_eval = 500;
    int eval = (int)(500 + (score * 2.5f));
    eval = (int)(0.7f * last_eval + 0.3f * eval);
    last_eval = eval;

    eval += (rand() % 7) - 3; // bruit ±3
    if (eval < 1) eval = 1;
    if (eval > 1000) eval = 1000;
    return eval;
}


// ===============================
// IA FAMINE (contrôle + opportunisme)
// ===============================
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
    float phase = 1.0f - (total / 48.0f);
    float score = 0.0f;

    // === noyau famine : pression et contrôle ===
    score += 40.0f * (casesVidesA - casesVidesJ);
    score += 25.0f * (famine1A - famine1J);
    score += 10.0f * (coupsJ - coupsA);
    score += 10.0f * (captJ - captA);
    score += 6.0f  * (grainesJ - grainesA);
    score += phase * 20.0f * ((casesVidesA + famine1A) - (casesVidesJ + famine1J));

    // === bonus et malus contextuels ===
    if (grainesA <= 6) score += (30 - 3 * grainesA);
    if (coupsA <= 2)   score += 25;
    if (casesVidesA >= 6) score += 20;

    if (grainesJ <= 6) score -= (30 - 3 * grainesJ);
    if (coupsJ <= 2)   score -= 25;
    if (casesVidesJ >= 6) score -= 20;

    // === mode agressif si on domine ===
    if (grainesA < grainesJ - 6) {
        score += 15.0f * (captJ - captA);
        score += 8.0f  * (coupsJ - coupsA);
    }

    if (grainesJ > grainesA + 8) {
        score += 0.5f * (grainesJ - grainesA);
    }

    // === stabilisation + léger bruit ===
    if (score < -250) score = -250;
    if (score > 250)  score = 250;

    static int last_eval_famine = 500;
    int eval = (int)(500 + (score * 2.0f));
    eval = (int)(0.7f * last_eval_famine + 0.3f * eval);
    last_eval_famine = eval;

    eval += (rand() % 7) - 3; // bruit ±3
    if (eval < 1) eval = 1;
    if (eval > 1000) eval = 1000;
    return eval;
}
