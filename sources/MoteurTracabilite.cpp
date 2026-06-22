/*
 * MoteurTracabilite.cpp
 *
 *  Created on: 23 mars 2026
 */
#include "MoteurTracabilite.h"
#include <iostream>
#include <algorithm>

// AJOUT : fonction pour extraire le nom du fichier depuis le chemin complet
static std::string nomFichier(const std::string& chemin) {
    size_t pos = chemin.find_last_of("/\\");
    if (pos != std::string::npos)
        return chemin.substr(pos + 1);
    return chemin;
}

//Constructeur : on initialise les taux à 0.0
MoteurTracabilite::MoteurTracabilite()
    : tauxSSS_SRS(0.0), tauxSRS_SDD(0.0), tauxGlobal(0.0),
    // AJOUT : initialisation du taux de traçabilité complète
    tauxTracabiliteComplete(0.0) {}

//On reçoit les 3 vecteurs du module 1 et on les stocke
void MoteurTracabilite::setDonnees(const std::vector<ExigenceSSS>& sss,
                                   const std::vector<ExigenceSRS>& srs,
                                   const std::vector<ExigenceSDD>& sdd) {
    this->exigencesSSS = sss;
    this->exigencesSRS = srs;
    this->exigencesSDD = sdd;
}

//Méthode privée : analyse SSS -> SRS
void MoteurTracabilite::analyserSSS_SRS() {

    for (const auto& srs : exigencesSRS) {
        for (const auto& ref : srs.getTracabilite()) {
            bool trouve = false;
            for (auto& sss : exigencesSSS) {
                if (sss.getId() == ref) {
                    sss.setStatut(Couverte);
                    trouve = true;
                    break;
                }
            }
            if (!trouve) {
                std::string fichierCible = "SSS";
                for (const auto& sss : exigencesSSS) {
                    const auto& alloue = sss.getAlloueA();
                    const auto& necSRS = srs.getNecessaireA();
                    for (const auto& na : necSRS) {
                        if (std::find(alloue.begin(), alloue.end(), na) != alloue.end()) {
                            fichierCible = nomFichier(sss.getFichierSource());
                            break;
                        }
                    }
                    if (fichierCible != "SSS") break;
                }
                liensBrises.push_back(
                    std::string("Lien brise : ") + srs.getId() +
                    " [" + nomFichier(srs.getFichierSource()) + "] reference " +
                    ref + " qui n'existe pas dans " + fichierCible
                    );
            }
        }
    }

    for (auto& sss : exigencesSSS) {
        if (sss.getStatut() == NonAnalyse) {
            sss.setStatut(NonCouverte);
        }
    }
}

//Méthode privée : analyse SRS -> SDD
void MoteurTracabilite::analyserSRS_SDD() {
    for (const auto& sdd : exigencesSDD) {
        for (const auto& ref : sdd.getReferencedSRS()) {
            std::cout << "  -> ref=" << ref << std::endl;
            bool trouve = false;
            for (auto& srs : exigencesSRS) {
                if (srs.getId() == ref) {
                    srs.setStatut(Couverte);
                    trouve = true;
                    break;
                }
            }
            if (!trouve) {
                // AJOUT : chercher le fichier SRS correspondant via l'ID référencé
                std::string fichierCible = "SRS";
                for (const auto& srs : exigencesSRS) {
                    if (srs.getId() == ref) {
                        fichierCible = nomFichier(srs.getFichierSource());
                        break;
                    }
                }
                liensBrises.push_back(
                    std::string("Lien brise : ") + sdd.getId() +
                    " [" + nomFichier(sdd.getFichierSource()) + "] reference " +
                    ref + " qui n'existe pas dans " + fichierCible
                    );
            }
        }
    }

    for (auto& srs : exigencesSRS) {
        if (srs.getStatut() == NonAnalyse) {
            srs.setStatut(NonCouverte);
        }
    }
}

void MoteurTracabilite::analyserTracabilite() {
    // Réinitialiser tous les statuts
    for (auto& sss : exigencesSSS) sss.setStatut(NonAnalyse);
    for (auto& srs : exigencesSRS) srs.setStatut(NonAnalyse);

    liensBrises.clear();
    orphelines.clear();

    analyserSSS_SRS();  // marque les SSS comme Couverte/NonCouverte
    analyserSRS_SDD();  // marque les SRS comme Couverte/NonCouverte

    calculerTaux();
}

void MoteurTracabilite::calculerTaux() {
    orphelines.clear(); // reset avant recalcul propre

    // ── Taux SSS→SRS ─────────────────────────────────────────────────────────
    if (exigencesSSS.empty()) {
        tauxSSS_SRS = 0.0;
        liensBrises.push_back("Avertissement : aucune exigence SSS trouvée");
    } else {
        int nbCouvertes = 0;
        for (const auto& sss : exigencesSSS)
            if (sss.getStatut() == Couverte) nbCouvertes++;
        tauxSSS_SRS = static_cast<double>(nbCouvertes) / static_cast<double>(exigencesSSS.size());
    }

    // ── Taux SRS→SDD ─────────────────────────────────────────────────────────
    if (exigencesSRS.empty()) {
        tauxSRS_SDD = 0.0;
        liensBrises.push_back("Avertissement : aucune exigence SRS trouvée");
    } else {
        int nbCouvertes = 0;
        for (const auto& srs : exigencesSRS)
            if (srs.getStatut() == Couverte) nbCouvertes++;
        tauxSRS_SDD = static_cast<double>(nbCouvertes) / static_cast<double>(exigencesSRS.size());
    }

    tauxGlobal = tauxSSS_SRS * tauxSRS_SDD;

    // ── Taux de traçabilité complète SSS→SRS→SDD ─────────────────────────────
    if (exigencesSSS.empty()) {
        tauxTracabiliteComplete = 0.0;
    } else {
        int nbCompletes = 0;
        for (const auto& sss : exigencesSSS) {
            bool chaineComplete = false;
            for (const auto& srs : exigencesSRS) {
                const auto& trac = srs.getTracabilite();
                bool srsCouvreSss = std::find(trac.begin(), trac.end(), sss.getId()) != trac.end();
                if (srsCouvreSss && srs.getStatut() == Couverte) {
                    chaineComplete = true;
                    break;
                }
            }
            if (chaineComplete) nbCompletes++;
        }
        tauxTracabiliteComplete = static_cast<double>(nbCompletes) /
                                  static_cast<double>(exigencesSSS.size());
    }

    // ── Orphelines ────────────────────────────────────────────────────────────
    // Orpheline SDD = IMPL_ sans aucune référence SRS
    for (const auto& sdd : exigencesSDD) {
        if (sdd.getReferencedSRS().empty()) {
            orphelines.push_back(
                std::string("Orpheline SDD : ") + sdd.getId() +
                " [" + nomFichier(sdd.getFichierSource()) + "]" +
                " ne reference aucune exigence SRS"
                );
        }
    }

    // Orpheline SRS = EXIGENCE_ sans aucune tracabilité SSS
    for (const auto& srs : exigencesSRS) {
        if (srs.getTracabilite().empty()) {
            orphelines.push_back(
                std::string("Orpheline SRS : ") + srs.getId() +
                " [" + nomFichier(srs.getFichierSource()) + "]" +
                " ne reference aucune exigence SSS"
                );
        }
    }

    // Retirer des orphelines les SRS déjà comptés comme liens brisés
    // Un SRS avec tracabilité vers SSS inexistant = lien brisé uniquement, pas orpheline
    orphelines.erase(
        std::remove_if(orphelines.begin(), orphelines.end(),
                       [this](const std::string& orph) {
                           // Extraire l'ID de l'orpheline depuis la chaîne
                           size_t debut = orph.find(": ") + 2;
                           size_t fin   = orph.find(" [");
                           if (debut == std::string::npos || fin == std::string::npos) return false;
                           std::string idOrph = orph.substr(debut, fin - debut);
                           // Vérifier si cet ID apparaît dans un lien brisé
                           for (const auto& lb : liensBrises) {
                               // Extraire l'ID exact du SRS depuis le lien brisé
                               // Format : "Lien brise : ID_SRS [fichier] reference ..."
                               size_t debut = lb.find(": ");
                               size_t fin   = lb.find(" [");
                               if (debut != std::string::npos && fin != std::string::npos) {
                                   std::string idDansLB = lb.substr(debut + 2, fin - debut - 2);
                                   if (idDansLB == idOrph) return true;
                               }
                           }
                           return false;
                       }),
        orphelines.end()
        );
}

//Méthodes de retour
double MoteurTracabilite::getTauxSSS_SRS() const { return tauxSSS_SRS; }
double MoteurTracabilite::getTauxSRS_SDD() const { return tauxSRS_SDD; }
double MoteurTracabilite::getTauxGlobal()  const { return tauxGlobal;  }
// AJOUT : getter pour le taux de traçabilité complète
double MoteurTracabilite::getTauxTracabiliteComplete() const { return tauxTracabiliteComplete; }

const std::vector<std::string>& MoteurTracabilite::getLiensBrises() const { return liensBrises; }
const std::vector<ExigenceSRS>& MoteurTracabilite::getExigencesSRS() const { return exigencesSRS; }
const std::vector<ExigenceSDD>& MoteurTracabilite::getExigencesSDD() const { return exigencesSDD; }
const std::vector<std::string>& MoteurTracabilite::getOrphelines()   const { return orphelines;  }

//Retour du rapport pour le module 3
RapportTracabilite MoteurTracabilite::getRapport() const {
    RapportTracabilite rapport;
    rapport.exigencesSSS            = exigencesSSS;
    rapport.liensBrises             = liensBrises;
    rapport.tauxSSS_SRS             = tauxSSS_SRS;
    rapport.tauxSRS_SDD             = tauxSRS_SDD;
    rapport.tauxGlobal              = tauxGlobal;
    // AJOUT : taux de traçabilité complète dans le rapport
    rapport.tauxTracabiliteComplete = tauxTracabiliteComplete;
    return rapport;
}
