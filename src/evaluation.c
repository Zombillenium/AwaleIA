#include "evaluation.h"
#include "plateau.h"  
#include "jeu.h"

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

    if (!plateau) return 0;

    // 1) Vérification de victoire réelle
    int res = evaluer_fin_de_partie(plateau, joueur);
    if (res == +1) return SCORE_VICTOIRE;
    if (res == -1) return SCORE_DEFAITE;

    // --- Heuristique famine classique ---
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

    // === noyau famine ===
    score += 40.0f * (casesVidesA - casesVidesJ);
    score += 25.0f * (famine1A - famine1J);

    // === mobilité et captures ===
    score += 10.0f * (coupsJ - coupsA);
    score += 10.0f * (captJ - captA);

    // === ressources ===
    score += 6.0f * (grainesJ - grainesA);

    // === pression fin de partie ===
    score += phase * 20.0f * ((casesVidesA + famine1A) - (casesVidesJ + famine1J));

    // === bonus si adversaire très faible ===
    if (grainesA <= 6) score += (30 - 3 * grainesA);
    if (coupsA <= 2)   score += 25;
    if (casesVidesA >= 6) score += 20;

    // === malus si nous sommes faibles ===
    if (grainesJ <= 6) score -= (30 - 3 * grainesJ);
    if (coupsJ <= 2)   score -= 25;
    if (casesVidesJ >= 6) score -= 20;

    // === agressif si on domine ===
    if (grainesA < grainesJ - 6) {
        score += 15.0f * (captJ - captA);
        score += 8.0f  * (coupsJ - coupsA);
    }

    // === prise de risque si en avance ===
    if (grainesJ > grainesA + 8) {
        score += 0.5f * (grainesJ - grainesA);
    }

    return (int)score;
}

// =============================
// FONCTIONS UTILITAIRES PATTERNS
// =============================

float calculer_phase_jeu(Plateau* plateau) {
    int total = total_graines(plateau);
    // Phase 0.0 = début (48 graines), 1.0 = fin (0 graines)
    float phase = 1.0f - (total / 48.0f);
    if (phase < 0.0f) phase = 0.0f;
    if (phase > 1.0f) phase = 1.0f;
    return phase;
}

int detecter_menace_famine(Plateau* plateau, int joueur) {
    // Détecte si on peut affamer l'adversaire en un coup
    int adversaire = 3 - joueur;
    int graines_adversaire = total_graines_joueur(plateau, adversaire);
    
    if (graines_adversaire == 0) return 0; // déjà affamé
    
    // Vérifie si on peut capturer toutes les graines adverses
    Plateau* p = plateau;
    int menace = 0;
    do {
        if (case_du_joueur(p->caseN, joueur)) {
            int total = p->R + p->B + p->T;
            // Si on peut distribuer assez pour capturer toutes les graines adverses
            if (total >= graines_adversaire) {
                menace = 1;
                break;
            }
        }
        p = p->caseSuiv;
    } while (p != plateau);
    
    return menace;
}

int compter_cases_critiques(Plateau* plateau, int joueur) {
    // Cases avec 1 graine (vulnérables) ou 0 (déjà vides)
    int critiques = 0;
    Plateau* p = plateau;
    do {
        if (case_du_joueur(p->caseN, joueur)) {
            int total = p->R + p->B + p->T;
            if (total <= 1) critiques++;
        }
        p = p->caseSuiv;
    } while (p != plateau);
    return critiques;
}

// =============================
// ÉVALUATION AVANCÉE CLASSIQUE
// =============================

int evaluer_plateau_avance(Plateau* plateau, int joueur) {
    int totalJ = 0, totalA = 0;
    int transJ = 0, transA = 0;
    int casesCapturablesJ = 0, casesCapturablesA = 0;
    int casesVidesJ = 0, casesVidesA = 0;
    int coupsLegauxJ = 0, coupsLegauxA = 0;

    Plateau* p = plateau;
    do {
        int total = p->R + p->B + p->T;
        if (case_du_joueur(p->caseN, joueur)) {
            totalJ += total;
            transJ += p->T;
            if (total == 2 || total == 3) casesCapturablesJ++;
            if (total == 0) casesVidesJ++;
            if (p->R > 0 || p->B > 0 || p->T > 0) coupsLegauxJ++;
        } else {
            totalA += total;
            transA += p->T;
            if (total == 2 || total == 3) casesCapturablesA++;
            if (total == 0) casesVidesA++;
            if (p->R > 0 || p->B > 0 || p->T > 0) coupsLegauxA++;
        }
        p = p->caseSuiv;
    } while (p != plateau);

    float phase = calculer_phase_jeu(plateau);
    
    // === PATTERNS AWALÉ ===
    int menace_famine = detecter_menace_famine(plateau, joueur);
    int cases_critiques_j = compter_cases_critiques(plateau, joueur);
    int cases_critiques_a = compter_cases_critiques(plateau, 3 - joueur);
    
    float score = 0.0f;
    
    // === RESSOURCES (pondération selon phase) ===
    float poids_ressources = 1.0f + phase * 0.5f; // Plus important en fin de partie
    score += poids_ressources * 3.0f * totalJ;
    score -= poids_ressources * 2.0f * totalA;
    
    // === MOBILITÉ (pondération selon phase) ===
    float poids_mobilite = 2.0f - phase * 1.0f; // Plus important en début de partie
    score += poids_mobilite * 4.0f * (coupsLegauxJ - coupsLegauxA);
    
    // === CAPTURES IMMINENTES ===
    score += 6.6f * (casesCapturablesJ - casesCapturablesA); // Tuned: 6.00 → 6.60
    
    // === GRAINES TRANSPARENTES ===
    score += 1.5f * (transJ - transA);
    
    // === PATTERNS BONUS/MALUS ===
    if (menace_famine) score += 40.0f; // Menace de famine = très bon (Tuned: 50.00 → 40.00)
    score -= 15.0f * cases_critiques_j; // Cases critiques = vulnérabilité
    score += 15.0f * cases_critiques_a; // Adversaire vulnérable = bon
    
    // === CASES VIDES (pattern important) ===
    score += 8.0f * (casesVidesA - casesVidesJ);
    
    // === Mise à l'échelle dans [1, 1000] ===
    if (score < -200.0f) score = -200.0f;
    if (score > 200.0f) score = 200.0f;
    
    int eval = (int)(500.0f + (score * 2.5f));
    if (eval < 1) eval = 1;
    if (eval > 1000) eval = 1000;
    
    return eval;
}

// =============================
// ÉVALUATION AVANCÉE FAMINE
// =============================

int evaluer_plateau_famine_avance(Plateau* plateau, int joueur) {
    if (!plateau) return 0;

    // Vérification de victoire réelle
    int res = evaluer_fin_de_partie(plateau, joueur);
    if (res == SCORE_VICTOIRE) return SCORE_VICTOIRE;
    if (res == SCORE_DEFAITE) return SCORE_DEFAITE;

    int grainesJ = 0, grainesA = 0;
    int casesVidesJ = 0, casesVidesA = 0;
    int coupsJ = 0, coupsA = 0;
    int captJ = 0, captA = 0;
    int famine1J = 0, famine1A = 0;
    int coupsLegauxJ = 0, coupsLegauxA = 0;

    Plateau* p = plateau;
    do {
        int total = p->R + p->B + p->T;

        if (case_du_joueur(p->caseN, joueur)) {
            grainesJ += total;
            if (total == 0) casesVidesJ++;
            if (total > 0) coupsJ++;
            if (total == 2 || total == 3) captJ++;
            if (total == 1) famine1J++;
            if (p->R > 0 || p->B > 0 || p->T > 0) coupsLegauxJ++;
        } else {
            grainesA += total;
            if (total == 0) casesVidesA++;
            if (total > 0) coupsA++;
            if (total == 2 || total == 3) captA++;
            if (total == 1) famine1A++;
            if (p->R > 0 || p->B > 0 || p->T > 0) coupsLegauxA++;
        }

        p = p->caseSuiv;
    } while (p != plateau);

    float phase = calculer_phase_jeu(plateau);
    
    // === PATTERNS AWALÉ ===
    int menace_famine = detecter_menace_famine(plateau, joueur);
    int cases_critiques_j = compter_cases_critiques(plateau, joueur);
    int cases_critiques_a = compter_cases_critiques(plateau, 3 - joueur);
    
    float score = 0.0f;

    // === NOYAU FAMINE (toujours important) ===
    score += 40.0f * (casesVidesA - casesVidesJ);
    score += 25.0f * (famine1A - famine1J);

    // === MOBILITÉ vs RESSOURCES (selon phase) ===
    float poids_mobilite = 2.0f - phase * 1.2f; // Priorité mobilité en début
    float poids_ressources = 0.5f + phase * 1.0f; // Priorité ressources en fin
    
    score += poids_mobilite * 12.0f * (coupsLegauxJ - coupsLegauxA);
    score += poids_ressources * 8.0f * (grainesJ - grainesA);
    
    // === CAPTURES ===
    score += 12.0f * (captJ - captA);

    // === PRESSION FIN DE PARTIE ===
    score += phase * 25.0f * ((casesVidesA + famine1A) - (casesVidesJ + famine1J));

    // === PATTERNS BONUS/MALUS ===
    if (menace_famine) score += 60.0f; // Menace de famine = excellent
    score -= 20.0f * cases_critiques_j; // Cases critiques = très vulnérable
    score += 20.0f * cases_critiques_a; // Adversaire vulnérable = excellent
    
    // === BONUS SI ADVERSAIRE TRÈS FAIBLE ===
    if (grainesA <= 6) score += (35 - 3 * grainesA);
    if (coupsLegauxA <= 2) score += 30;
    if (casesVidesA >= 6) score += 25;

    // === MALUS SI NOUS SOMMES FAIBLES ===
    if (grainesJ <= 6) score -= (35 - 3 * grainesJ);
    if (coupsLegauxJ <= 2) score -= 30;
    if (casesVidesJ >= 6) score -= 25;

    // === AGRESSIF SI ON DOMINE ===
    if (grainesA < grainesJ - 6) {
        score += 18.0f * (captJ - captA);
        score += 10.0f * (coupsLegauxJ - coupsLegauxA);
    }

    // === PRISE DE RISQUE SI EN AVANCE ===
    if (grainesJ > grainesA + 8) {
        score += 0.6f * (grainesJ - grainesA);
    }

    return (int)score;
}
