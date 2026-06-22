/*
 * MoteurTracabilite.h
 *
 *  Created on: 10 mars 2026
 */

#ifndef MOTEURTRACABILITE_H_
#define MOTEURTRACABILITE_H_

#include "ExigenceSSS.h"
#include "ExigenceSRS.h"
#include "ExigenceSDD.h"
#include "RapportTracabilite.h"
#include <vector>
#include <string>

class MoteurTracabilite {

private:
    //Les 3 vecteurs d'exigences reçus du module 1
    std::vector<ExigenceSSS> exigencesSSS;
    std::vector<ExigenceSRS> exigencesSRS;
    std::vector<ExigenceSDD> exigencesSDD;

    //Les liens brisés détectés pendant l'analyse
    std::vector<std::string> liensBrises;
    // AJOUT : les orphelines détectées pendant l'analyse (séparées des liens brisés)
    std::vector<std::string> orphelines;

    //Les taux calculés
    double tauxSSS_SRS;
    double tauxSRS_SDD;
    double tauxGlobal;

    //Méthodes qui nous permettera d'exécuter les 2 traçabilité
    void analyserSSS_SRS();  // analyse uniquement SSS -> SRS
    void analyserSRS_SDD();  // analyse uniquement SRS -> SDD


public:
    //Constructeur
    MoteurTracabilite();


    //Méthodes

    //Reçoit les données du module 1
    void setDonnees(const std::vector<ExigenceSSS>& sss,
                    const std::vector<ExigenceSRS>& srs,
                    const std::vector<ExigenceSDD>& sdd);

    //Lance l'analyse de traçabilité
    void analyserTracabilite();

    //Calcule les taux de couverture
    void calculerTaux();

    //Méthodes de retour des taux
    double getTauxSSS_SRS() const;
    double getTauxSRS_SDD() const;
    double getTauxGlobal() const;
    // AJOUT : taux de traçabilité complète
    double tauxTracabiliteComplete;

    // AJOUT : getter
    double getTauxTracabiliteComplete() const;

    //Méthode de retour des liens brisés
    const std::vector<std::string>& getLiensBrises() const;
    // AJOUT : retourne les orphelines séparément
    const std::vector<std::string>& getOrphelines() const;

    const std::vector<ExigenceSRS>& getExigencesSRS() const;
    const std::vector<ExigenceSDD>& getExigencesSDD() const;



    //Méthode de retour du rapport complet pour le module 3
    RapportTracabilite getRapport() const;
};

#endif /* MOTEURTRACABILITE_H_ */
