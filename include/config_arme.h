#ifndef CONFIG_ARME_H
#define CONFIG_ARME_H

/*
 * config_arme.h
 * -------------
 * Charge une base d'armes depuis un fichier INI simple + section PLAYER.
 * Portable Linux / Windows (C standard).
 *
 * Section joueur (optionnelle):
 *   [PLAYER]
 *   name=TonNom
 *
 * Section arme:
 *   [Nom Exact de l'arme]
 *   dpp=...
 *   ammo_shot=...
 *   decay_shot=...
 *   amp_decay_shot=...
 *   markup=...
 *   notes=...
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
    #endif
    
    typedef struct arme_stats
    {
        char  name[128];         /* Nom de l'arme (section INI) */
        char  notes[256];        /* Texte libre */
        
        double dpp;              /* Damage Per PEC (ou DPP) */
        double ammo_shot;        /* Coût munition par tir (PED) */
        double decay_shot;       /* Decay arme par tir (PED) */
        double amp_decay_shot;   /* Decay ampli par tir (PED) */
        
        /* Legacy */
        double markup;           /* Multiplicateur MU: 1.00=TT, 1.25=125% */
        
        /* NEW: MU séparés (EU) */
        double  ammo_mu;         /* ex: 1.00 */
        double  weapon_mu;       /* MU de l'arme (L) */
        double  amp_mu;          /* MU de l'amp (L) */
        
    } arme_stats;
    
    typedef struct armes_db
    {
        arme_stats *items;
        size_t count;
        
        /* AJOUT: nom du joueur lu dans [PLAYER] name=... */
        char player_name[128];
    } armes_db;
    
    /* Charge config/armes.ini (ou autre chemin).
     * Retour: 1 si OK, 0 si erreur.
     */
    int armes_db_load(armes_db *db, const char *path);
    
    /* Libère la mémoire. */
    void armes_db_free(armes_db *db);
    
    /* Recherche une arme par nom exact (strcmp).
     * Retour: pointeur vers l'arme, ou NULL.
     */
    const arme_stats *armes_db_find(const armes_db *db, const char *name);
    
    /* Calcul coût total par tir (PED), avec MU appliqué.
     * cost_shot = (ammo + decay + amp_decay) * markup
     */
    double arme_cost_shot(const arme_stats *w);
    
    #ifdef __cplusplus
}
#endif

#endif /* CONFIG_ARME_H */