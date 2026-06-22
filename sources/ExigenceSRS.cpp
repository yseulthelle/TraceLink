/*
 * ExigenceSRS.cpp
 *
 *  Created on: 9 mars 2026
 */

#include "ExigenceSRS.h"

/// @brief Appelle le constructeur de @c Exigence avec le type @c SRS.
/// Les trois vecteurs sont vides par défaut.


ExigenceSRS::ExigenceSRS(const std::string& id, const std::string& contenu)
    : Exigence(id, contenu, SRS), developpe(false) {}
/// @brief Retourne les identifiants SSS couverts par cette exigence.
const std::vector<std::string>& ExigenceSRS::getTracabilite() const {
    return tracabilite;
}

/// @brief Retourne les cibles de cette exigence.
const std::vector<std::string>& ExigenceSRS::getCibles() const {
    return cibles;
}

/// @brief Retourne les projets concernés par cette exigence.
const std::vector<std::string>& ExigenceSRS::getNecessaireA() const {
    return necessaireA;
}

/// @brief Modifie la liste de traçabilité vers les SSS.
void ExigenceSRS::setTracabilite(const std::vector<std::string>& tracabilite) {
    this->tracabilite = tracabilite;
}

/// @brief Modifie la liste des cibles.
void ExigenceSRS::setCibles(const std::vector<std::string>& cibles) {
    this->cibles = cibles;
}

/// @brief Modifie la liste des projets concernés.
void ExigenceSRS::setNecessaireA(const std::vector<std::string>& necessaireA) {
    this->necessaireA = necessaireA;
}

bool ExigenceSRS::getDeveloppe() const { return developpe; }
void ExigenceSRS::setDeveloppe(bool d) { developpe = d; }

const std::string& ExigenceSRS::getEtat() const { return etat; }
void ExigenceSRS::setEtat(const std::string& e) { etat = e; }
const std::string& ExigenceSRS::getMethodeVerification() const { return methodeVerification; }
void ExigenceSRS::setMethodeVerification(const std::string& m) { methodeVerification = m; }

