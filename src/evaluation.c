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

    float phase = calculer_phase_jeu(plateau); // Utiliser la fonction améliorée

    float score = 0.0f;

    // === noyau famine ===
    score += 40.0f * (casesVidesA - casesVidesJ);
    score += 25.0f * (famine1A - famine1J);

    // === mobilité et captures ===
    // Pour parties longues, mobilité plus importante en début
    float poids_mobilite = 1.2f - phase * 0.4f; // Plus important en début
    score += poids_mobilite * 10.0f * (coupsJ - coupsA);
    // Captures moins critiques en début de partie longue
    float poids_captures = 0.85f + phase * 0.15f;
    score += poids_captures * 10.0f * (captJ - captA);

    // === ressources ===
    // Ressources importantes pour tenir sur la longueur
    float poids_ressources = 0.8f + phase * 0.4f;
    score += poids_ressources * 6.0f * (grainesJ - grainesA);

    // === pression fin de partie ===
    // Transition plus douce pour parties longues
    score += phase * phase * 20.0f * ((casesVidesA + famine1A) - (casesVidesJ + famine1J));

    // === bonus si adversaire très faible ===
    // Pour parties longues, être plus patient - seuils ajustés selon phase
    float seuil_graines = 6.0f - phase * 1.5f;
    float seuil_coups = 2.0f - phase * 0.5f;
    float seuil_cases = 6.0f - phase * 1.0f;
    
    if (grainesA <= (int)seuil_graines) {
        float bonus = (30 - 3 * grainesA) * (0.7f + phase * 0.3f);
        score += bonus;
    }
    if (coupsA <= (int)seuil_coups) {
        float bonus = 25.0f * (0.7f + phase * 0.3f);
        score += bonus;
    }
    if (casesVidesA >= (int)seuil_cases) {
        float bonus = 20.0f * (0.7f + phase * 0.3f);
        score += bonus;
    }

    // === malus si nous sommes faibles ===
    if (grainesJ <= (int)seuil_graines) {
        float malus = (30 - 3 * grainesJ) * (0.7f + phase * 0.3f);
        score -= malus;
    }
    if (coupsJ <= (int)seuil_coups) {
        float malus = 25.0f * (0.7f + phase * 0.3f);
        score -= malus;
    }
    if (casesVidesJ >= (int)seuil_cases) {
        float malus = 20.0f * (0.7f + phase * 0.3f);
        score -= malus;
    }

    // === agressif si on domine ===
    // Plus agressif seulement en fin de partie ou si on domine clairement
    float seuil_domination = 6.0f - phase * 2.0f;
    if (grainesA < grainesJ - seuil_domination) {
        float agressivite = 0.6f + phase * 0.4f;
        score += agressivite * 15.0f * (captJ - captA);
        score += agressivite * 8.0f * (coupsJ - coupsA);
    }

    // === prise de risque si en avance ===
    // Plus conservateur en début de partie longue
    float seuil_avance = 8.0f + phase * 2.0f;
    if (grainesJ > grainesA + seuil_avance) {
        float risque = 0.3f + phase * 0.2f;
        score += risque * (grainesJ - grainesA);
    }

    return (int)score;
}

// =============================
// FONCTIONS UTILITAIRES PATTERNS
// =============================

float calculer_phase_jeu(Plateau* plateau) {
    int total = total_graines(plateau);
    // Phase 0.0 = début (48 graines), 1.0 = fin (0 graines)
    // Pour parties longues (400 coups max), phase progresse plus lentement
    // pour mieux refléter les phases intermédiaires plus longues
    float phase = 1.0f - (total / 48.0f);
    // Ajustement : phase progresse plus lentement (carré pour ralentir)
    // Mélange linéaire (20%) et quadratique (80%) pour une transition plus douce
    phase = phase * phase * 0.8f + phase * 0.2f;
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
    // Pour parties longues (400 coups), ressources importantes tout au long
    float poids_ressources = 1.0f + phase * 0.6f; // Plus important en fin, mais aussi en milieu
    score += poids_ressources * 3.0f * totalJ;
    score -= poids_ressources * 2.0f * totalA;
    
    // === MOBILITÉ (pondération selon phase) ===
    // Pour parties longues, mobilité critique en début/milieu (plus de temps pour construire)
    float poids_mobilite = 2.5f - phase * 1.2f; // Augmenté de 2.0 → 2.5 pour début
    score += poids_mobilite * 4.0f * (coupsLegauxJ - coupsLegauxA);
    
    // === CAPTURES IMMINENTES ===
    // Légèrement réduit en début de partie longue (moins critique qu'en partie courte)
    float poids_captures = 0.8f + phase * 0.2f; // 0.8 en début, 1.0 en fin
    score += poids_captures * 6.6f * (casesCapturablesJ - casesCapturablesA);
    
    // === GRAINES TRANSPARENTES ===
    score += 1.5f * (transJ - transA);
    
    // === PATTERNS BONUS/MALUS ===
    if (menace_famine) score += 40.0f; // Menace de famine = très bon (Tuned: 50.00 → 40.00)
    // Pour parties longues, être moins pénalisé par cases critiques en début
    float poids_critiques = 0.8f + phase * 0.2f; // Moins pénalisant en début
    score -= poids_critiques * 15.0f * cases_critiques_j; // Cases critiques = vulnérabilité
    score += poids_critiques * 15.0f * cases_critiques_a; // Adversaire vulnérable = bon
    
    // === CASES VIDES (pattern important) ===
    // Plus important en fin de partie pour parties longues
    float poids_vides = 0.7f + phase * 0.3f; // Moins important en début
    score += poids_vides * 8.0f * (casesVidesA - casesVidesJ);
    
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
    // Pour parties longues (400 coups), mobilité critique en début pour éviter blocage
    float poids_mobilite = 2.3f - phase * 1.3f; // Augmenté de 2.0 → 2.3 pour début
    // Ressources importantes pour tenir sur la longueur
    float poids_ressources = 0.6f + phase * 1.1f; // Légèrement augmenté
    
    score += poids_mobilite * 12.0f * (coupsLegauxJ - coupsLegauxA);
    score += poids_ressources * 8.0f * (grainesJ - grainesA);
    
    // === CAPTURES ===
    // Légèrement réduit en début de partie longue
    float poids_captures = 0.85f + phase * 0.15f; // 0.85 en début, 1.0 en fin
    score += poids_captures * 12.0f * (captJ - captA);

    // === PRESSION FIN DE PARTIE ===
    // Transition plus douce pour parties longues
    score += phase * phase * 25.0f * ((casesVidesA + famine1A) - (casesVidesJ + famine1J));

    // === PATTERNS BONUS/MALUS ===
    if (menace_famine) score += 60.0f; // Menace de famine = excellent
    score -= 20.0f * cases_critiques_j; // Cases critiques = très vulnérable
    score += 20.0f * cases_critiques_a; // Adversaire vulnérable = excellent
    
    // === BONUS SI ADVERSAIRE TRÈS FAIBLE ===
    // Pour parties longues, être plus patient - seuils ajustés selon phase
    float seuil_graines = 6.0f - phase * 1.5f; // Plus patient en début (seuil plus bas)
    float seuil_coups = 2.0f - phase * 0.5f;   // Plus patient en début
    float seuil_cases = 6.0f - phase * 1.0f;  // Plus patient en début
    
    if (grainesA <= (int)seuil_graines) {
        float bonus = (35 - 3 * grainesA) * (0.7f + phase * 0.3f); // Moins agressif en début
        score += bonus;
    }
    if (coupsLegauxA <= (int)seuil_coups) {
        float bonus = 30.0f * (0.7f + phase * 0.3f); // Moins agressif en début
        score += bonus;
    }
    if (casesVidesA >= (int)seuil_cases) {
        float bonus = 25.0f * (0.7f + phase * 0.3f); // Moins agressif en début
        score += bonus;
    }

    // === MALUS SI NOUS SOMMES FAIBLES ===
    if (grainesJ <= (int)seuil_graines) {
        float malus = (35 - 3 * grainesJ) * (0.7f + phase * 0.3f);
        score -= malus;
    }
    if (coupsLegauxJ <= (int)seuil_coups) {
        float malus = 30.0f * (0.7f + phase * 0.3f);
        score -= malus;
    }
    if (casesVidesJ >= (int)seuil_cases) {
        float malus = 25.0f * (0.7f + phase * 0.3f);
        score -= malus;
    }

    // === AGRESSIF SI ON DOMINE ===
    // Plus agressif seulement en fin de partie ou si on domine clairement
    float seuil_domination = 6.0f - phase * 2.0f; // Plus patient en début
    if (grainesA < grainesJ - seuil_domination) {
        float agressivite = 0.6f + phase * 0.4f; // Plus agressif en fin
        score += agressivite * 18.0f * (captJ - captA);
        score += agressivite * 10.0f * (coupsLegauxJ - coupsLegauxA);
    }

    // === PRISE DE RISQUE SI EN AVANCE ===
    // Plus conservateur en début de partie longue
    float seuil_avance = 8.0f + phase * 2.0f; // Plus patient en début (seuil plus haut)
    if (grainesJ > grainesA + seuil_avance) {
        float risque = 0.4f + phase * 0.2f; // Moins risqué en début
        score += risque * (grainesJ - grainesA);
    }

    return (int)score;
}
