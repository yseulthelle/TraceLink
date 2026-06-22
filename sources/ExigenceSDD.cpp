/*
 * ExigenceSDD.cpp
 *
 *  Created on: 9 mars 2026
 */

#include "ExigenceSDD.h"

/// @brief Appelle le constructeur de @c Exigence avec le type @c SDD.
ExigenceSDD::ExigenceSDD(const std::string& id, const std::string& contenu)
    : Exigence(id, contenu, SDD), developpe(false) {}

/// @brief Retourne les identifiants SRS référencés par cette exigence.
const std::vector<std::string>& ExigenceSDD::getReferencedSRS() const {
    return referencedSRS;
}

/// @brief Modifie la liste des SRS référencées.
void ExigenceSDD::setReferencedSRS(const std::vector<std::string>& referencedSRS) {
    this->referencedSRS = referencedSRS;
}
bool ExigenceSDD::getDeveloppe() const { return developpe; }
void ExigenceSDD::setDeveloppe(bool d) { developpe = d; }


