# Awalé IA - Intelligence Artificielle pour le Jeu d'Awalé

## Table des matières
1. [Les Deux Stratégies d'IA](#les-deux-stratégies-dia)
2. [Déroulement d'une Partie](#déroulement-dune-partie)
3. [Algorithmes Utilisés](#algorithmes-utilisés)
4. [Compilation et Exécution](#compilation-et-exécution)

---

## Les Deux Stratégies d'IA

### Stratégie 1 : IA Classique (`evaluer_plateau_avance`)

**Philosophie** : Maximiser les captures tout en maintenant une bonne mobilité.

#### Critères d'évaluation

| Critère | Pondération | Description |
|---------|-------------|-------------|
| **Ressources (graines)** | `+3.0 × grainesJ` / `-2.0 × grainesA` | Avantage si on a plus de graines |
| **Mobilité** | `+4.0 × (coupsJ - coupsA)` | Nombre de coups légaux disponibles |
| **Cases capturables** | `+6.6 × (captJ - captA)` | Cases avec 2-3 graines (captures potentielles) |
| **Graines transparentes** | `+1.5 × (transJ - transA)` | Flexibilité de jeu |
| **Menace de famine** | `+40 points` | Bonus si on peut affamer l'adversaire |
| **Cases critiques** | `-15 × critiquesJ` / `+15 × critiquesA` | Pénalité pour cases vulnérables (0-1 graine) |
| **Cases vides** | `+8.0 × (videsA - videsJ)` | Avantage si l'adversaire a des cases vides |

#### Adaptation selon la phase de jeu

```
Phase = 1.0 - (total_graines / 48.0)
      = 0.0 en début de partie → 1.0 en fin de partie
```

- **Début de partie** : Focus sur la mobilité (pondération ×2.5)
- **Fin de partie** : Focus sur les ressources (pondération ×1.6)

#### Avantages de la stratégie Classique
- Agressive, favorise les captures immédiates
- Bonne gestion des ressources sur la durée
- Équilibrée entre offense et défense
- Détecte bien les menaces de famine

#### Inconvénients
- Peut être prévisible
- Moins efficace dans les situations de blocage

---

### Stratégie 2 : IA Famine (`evaluer_plateau_famine_avance`)

**Philosophie** : Asphyxier progressivement l'adversaire en vidant ses cases.

#### Critères d'évaluation

| Critère | Pondération | Description |
|---------|-------------|-------------|
| **Cases vides adverses** | `+40.0 × (videsA - videsJ)` | Critère PRINCIPAL |
| **Cases à 1 graine (famine imminente)** | `+25.0 × (famine1A - famine1J)` | Cases vulnérables |
| **Mobilité** | `+12.0 × (coupsLegauxJ - coupsLegauxA)` | Plus important en début |
| **Captures** | `+12.0 × (captJ - captA)` | Cases avec 2-3 graines |
| **Ressources** | `+8.0 × (grainesJ - grainesA)` | Ressources de base |
| **Menace de famine** | `+60 points` | Bonus TRÈS élevé |
| **Cases critiques** | `-20 × critiquesJ` / `+20 × critiquesA` | Pénalité accentuée |

#### Bonus spéciaux Famine

```c
// Si l'adversaire est très faible (peu de graines/coups)
if (grainesA <= 6) → bonus de 35 - 3×grainesA points
if (coupsLegauxA <= 2) → bonus de 30 points
if (casesVidesA >= 6) → bonus de 25 points
```

#### Mode agressif en fin de partie

Quand on domine (grainesJ > grainesA + 6) :
- Bonus captures : `+18 × (captJ - captA)`
- Bonus mobilité : `+10 × (coupsLegauxJ - coupsLegauxA)`

#### Avantages de la stratégie Famine
- Très efficace pour acculer l'adversaire
- Stratégie de "strangulation" progressive
- Excellente gestion des fins de partie
- Détection avancée des positions gagnantes

#### Inconvénients
- Peut négliger des captures immédiates profitables
- Début de partie parfois plus lent
- Nécessite une bonne vision à long terme

---

## Déroulement d'une Partie

### Protocole de communication avec l'Arbitre

```
┌─────────────┐         ┌──────────────┐         ┌─────────────┐
│  Arbitre    │         │    IA A      │         │    IA B     │
└──────┬──────┘         └──────┬───────┘         └──────┬──────┘
       │                       │                        │
       │──── START ──────────>│                        │
       │                       │ (calcul minimax)       │
       │<──── "3R" ───────────│                        │
       │                       │                        │
       │─────────────── "3R" ──────────────────────────>│
       │                       │                        │ (calcul minimax)
       │<────────────── "5B" ──────────────────────────│
       │                       │                        │
       │──── "5B" ──────────>│                        │
       │                       │ (calcul minimax)       │
       │<──── "7TR" ──────────│                        │
       │                       │                        │
       │         ... (jusqu'à 400 coups) ...           │
       │                       │                        │
       │<── "RESULT 8B 51 45" ─│                        │
```

### Format des coups

| Format | Signification |
|--------|---------------|
| `3R` | Case 3, graines Rouges |
| `5B` | Case 5, graines Bleues |
| `7TR` | Case 7, Transparentes jouées comme Rouges |
| `9TB` | Case 9, Transparentes jouées comme Bleues |

### Message de fin

```
RESULT <dernier_coup> <score_J1> <score_J2>
RESULT LIMIT <score_J1> <score_J2>  // si 400 coups atteints
```

---

## Algorithmes Utilisés

### 1. Minimax avec Alpha-Beta Pruning

```
minimax(position, profondeur, α, β, maximisant):
    if profondeur == 0 ou fin_de_partie:
        return évaluer(position)
    
    if maximisant:
        meilleur = -∞
        pour chaque coup possible:
            score = minimax(position', profondeur-1, α, β, false)
            meilleur = max(meilleur, score)
            α = max(α, score)
            if β ≤ α: break  # Coupe
        return meilleur
    else:
        meilleur = +∞
        pour chaque coup possible:
            score = minimax(position', profondeur-1, α, β, true)
            meilleur = min(meilleur, score)
            β = min(β, score)
            if β ≤ α: break  # Coupe
        return meilleur
```

### 2. Iterative Deepening (Approfondissement Itératif)

L'IA explore progressivement les profondeurs de 1 à 25, en s'arrêtant quand le temps (2s) est écoulé.

```
Profondeur 1  → temps: 0.001s
Profondeur 2  → temps: 0.003s
Profondeur 3  → temps: 0.010s
...
Profondeur 12 → temps: 1.850s  ← profondeur retenue si temps proche de 2s
```

### 3. Aspiration Windows

Technique pour accélérer la recherche en utilisant une fenêtre α-β réduite basée sur le score précédent.

```c
int ASP = 75;
alpha_root = score_précédent - ASP;
beta_root  = score_précédent + ASP;

// Si fail-low ou fail-high → on élargit la fenêtre
ASP *= 2;
```

### 4. Table de Transposition (Zobrist Hashing)

Cache de 50 000 entrées pour éviter de recalculer les positions déjà vues.

```c
hash = XOR de tous les zobrist[case][couleur][nb_graines]
```

### 5. Killer Moves

Mémorisation des 2 meilleurs coups par profondeur pour les tester en priorité.

```c
killer_moves[profondeur][0]  // Meilleur coup ayant causé une coupe
killer_moves[profondeur][1]  // Second meilleur
```

### 6. Move Ordering (Tri des coups)

Les cases sont triées par nombre total de graines (décroissant) pour explorer d'abord les coups les plus prometteurs.

---

## Compilation et Exécution

### Prérequis
- **GCC** avec support OpenMP
- **Java** (JDK 8+) pour l'arbitre
- **Windows** ou Linux

### Compilation

```bash
# Compilation avec Makefile
make

# Ou manuellement avec GCC
gcc -O3 -fopenmp -o main_eval.exe src/main_eval.c src/ia.c src/evaluation.c \
    src/jeu.c src/plateau.c src/tabletranspo.c -I include
```

### Exécution d'un match

```bash
# Compiler l'arbitre Java
javac rendu/Arbitre.java

# Lancer un match entre deux IA
cd rendu
java Arbitre
```

## Comparaison des Stratégies

| Aspect | IA Classique | IA Famine |
|--------|--------------|-----------|
| **Objectif principal** | Maximiser les captures | Vider les cases adverses |
| **Style de jeu** | Agressif, direct | Stratégique, étouffant |
| **Début de partie** | Focus mobilité | Focus contrôle |
| **Fin de partie** | Focus ressources | Focus pression famine |
| **Détection menaces** | Bonne (+40 pts) | Excellente (+60 pts) |
| **Contre adversaire agressif** | 3/5 | 4/5 |
| **Contre adversaire défensif** | 4/5 | 3/5 |
| **Parties longues (>200 coups)** | 3/5 | 5/5 |
| **Parties courtes (<100 coups)** | 4/5 | 3/5 |


