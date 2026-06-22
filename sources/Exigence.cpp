/*
 * Exigence.cpp
 *
 *  Created on: 9 mars 2026
 */
#include "Exigence.h"
/// @brief Initialise l'exigence avec son identifiant, son contenu et son type.
/// Le statut est positionné à @c NonAnalyse par défaut.
Exigence::Exigence(const std::string& id, const std::string& contenu, TypeExigence type)
    : id(id), contenu(contenu), type(type), statut(NonAnalyse) {}
/// @brief Retourne l'identifiant unique de l'exigence.
std::string Exigence::getId() const {
    return id;
}
/// @brief Retourne le contenu textuel de l'exigence.
std::string Exigence::getContenu() const {
    return contenu;
}
/// @brief Retourne le type hiérarchique de l'exigence.
TypeExigence Exigence::getType() const {
    return type;
}
/// @brief Retourne le statut de couverture de l'exigence.
Statut Exigence::getStatut() const {
    return statut;
}
/// @brief Modifie le statut de couverture de l'exigence.
void Exigence::setStatut(Statut statut) {
    this->statut = statut;
}
// AJOUT : getter et setter pour le fichier source
/// @brief Retourne le chemin du fichier source de l'exigence.
std::string Exigence::getFichierSource() const {
    return fichierSource;
}
/// @brief Modifie le chemin du fichier source de l'exigence.
void Exigence::setFichierSource(const std::string& fichier) {
    this->fichierSource = fichier;
}
