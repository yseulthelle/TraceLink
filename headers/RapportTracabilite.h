/*
 * RapportTracabilite.h
 *
 *  Created on: 10 mars 2026
 */
//module qui va permettre de transférer la structure de données contenant les infos sur la traçabilité au module 3
#ifndef RAPPORTTRACABILITE_H_
#define RAPPORTTRACABILITE_H_
#include "ExigenceSSS.h"
#include <vector>
#include <string>

struct RapportTracabilite {
    std::vector<ExigenceSSS> exigencesSSS;
    std::vector<std::string> liensBrises;
    double tauxSSS_SRS;  ///< Taux de SSS couvertes par au moins une SRS
    double tauxSRS_SDD;  ///< Taux de SRS couvertes par au moins une SDD
    double tauxGlobal;   ///< Produit des deux taux intermédiaires

    // AJOUT : taux de traçabilité complète SSS→SRS→SDD
    double tauxTracabiliteComplete; ///< Nb SSS avec chaîne complète SSS→SRS→SDD / nb total SSS
};

#endif /* RAPPORTTRACABILITE_H_ */
