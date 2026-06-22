/*
 * Exigence.h
 *
 *  Created on: 9 mars 2026
 */
#ifndef EXIGENCE_H_
#define EXIGENCE_H_
#include <string>
/**
 * @enum TypeExigence
 * @brief Définit le niveau hiérarchique d'une exigence.
 *
 * @var TypeExigence::SSS
 *   Exigence client (System/Subsystem Specification).
 * @var TypeExigence::SRS
 *   Exigence projet (Software Requirements Specification).
 * @var TypeExigence::SDD
 *   Exigence d'implémentation (Software Design Document).
 */
enum TypeExigence {
    SSS,
    SRS,
    SDD
};
/**
 * @enum Statut
 * @brief Définit l'état de couverture d'une exigence après analyse de traçabilité.
 *
 * @var Statut::NonAnalyse
 *   L'exigence n'a pas encore été traitée par le moteur de traçabilité.
 * @var Statut::Couverte
 *   Au moins une exigence du niveau suivant référence cette exigence.
 * @var Statut::NonCouverte
 *   Aucune exigence du niveau suivant ne couvre cette exigence.
 */
enum Statut {
    NonAnalyse,
    Couverte,
    NonCouverte
};
/**
 * @class Exigence
 * @brief Classe mère abstraite représentant une exigence générique.
 *
 * Contient les attributs et comportements communs à tous les types
 * d'exigences (SSS, SRS, SDD) : identifiant, contenu, type et statut.
 * Le destructeur est virtuel pour garantir une destruction correcte
 * via un pointeur de classe mère.
 */
class Exigence {
private:
    std::string id;       ///< Identifiant unique (ex. : "EXIGENCE_CLIENT_A_F01").
    std::string contenu;  ///< Texte descriptif de l'exigence.
    TypeExigence type;    ///< Niveau hiérarchique de l'exigence (SSS, SRS ou SDD).
    Statut statut;        ///< État de couverture de l'exigence.
    // AJOUT : chemin du fichier source de l'exigence
    std::string fichierSource; ///< Chemin du fichier d'origine de l'exigence.
public:
    /**
     * @brief Construit une exigence avec ses attributs de base.
     * @param id      Identifiant unique de l'exigence.
     * @param contenu Texte descriptif de l'exigence.
     * @param type    Niveau hiérarchique de l'exigence (SSS, SRS ou SDD).
     *
     * Le statut est initialisé à @c NonAnalyse par défaut.
     */
    Exigence(const std::string& id, const std::string& contenu, TypeExigence type);
    /// @brief Destructeur virtuel pour éviter les fuites mémoire via pointeur de classe mère.
    virtual ~Exigence() = default;
    /// @brief Retourne l'identifiant unique de l'exigence.
    /// @return Chaîne contenant l'identifiant.
    std::string getId() const;
    /// @brief Retourne le statut de couverture de l'exigence.
    /// @return Valeur de l'énumération @c Statut.
    Statut getStatut() const;
    /// @brief Retourne le type hiérarchique de l'exigence.
    /// @return Valeur de l'énumération @c TypeExigence.
    TypeExigence getType() const;
    /// @brief Retourne le contenu textuel de l'exigence.
    /// @return Chaîne décrivant l'exigence.
    std::string getContenu() const;
    /**
     * @brief Modifie le statut de couverture de l'exigence.
     * @param statut Nouveau statut à affecter.
     */
    void setStatut(Statut statut);
    // AJOUT : getter et setter pour le fichier source
    /// @brief Retourne le chemin du fichier source de l'exigence.
    /// @return Chaîne contenant le chemin du fichier d'origine.
    std::string getFichierSource() const;
    /// @brief Modifie le chemin du fichier source de l'exigence.
    /// @param fichier Chemin du fichier source.
    void setFichierSource(const std::string& fichier);
};
#endif /* EXIGENCE_H_ */
