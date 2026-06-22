/*
 * ExigenceSSS.cpp
 *
 *  Created on: 9 mars 2026
 */

#include "ExigenceSSS.h"

/// @brief Appelle le constructeur de @c Exigence avec le type @c SSS.
/// @c developpe est initialisé à @c false.
ExigenceSSS::ExigenceSSS(const std::string& id, const std::string& contenu)
    : Exigence(id, contenu, SSS), developpe(false) {}

/// @brief Retourne l'état de développement de l'exigence.
bool ExigenceSSS::getDeveloppe() const {
    return developpe;
}

/// @brief Retourne la liste des projets auxquels cette exigence est allouée.
const std::vector<std::string>& ExigenceSSS::getAlloueA() const {
    return alloueA;
}

/// @brief Modifie l'état de développement de l'exigence.
void ExigenceSSS::setDeveloppe(bool developpe) {
    this->developpe = developpe;
}

/// @brief Modifie la liste des projets auxquels l'exigence est allouée.
void ExigenceSSS::setAlloueA(const std::vector<std::string>& alloueA) {
    this->alloueA = alloueA;
}
const std::string& ExigenceSSS::getEtat() const { return etat; }
void ExigenceSSS::setEtat(const std::string& e) { etat = e; }
const std::string& ExigenceSSS::getMethodeVerification() const { return methodeVerification; }
void ExigenceSSS::setMethodeVerification(const std::string& m) { methodeVerification = m; }


