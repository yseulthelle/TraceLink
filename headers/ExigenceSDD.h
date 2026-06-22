/*
 * ExigenceSDD.h
 *
 *  Created on: 9 mars 2026
 */

#ifndef EXIGENCESDD_H_
#define EXIGENCESDD_H_

#include "Exigence.h"
#include <vector>

/**
 * @class ExigenceSDD
 * @brief Exigence de niveau SDD (Software Design Document).
 *
 * Hérite de @c Exigence. Contient @c referencedSRS, la liste des
 * identifiants SRS que cette exigence d'implémentation couvre.
 */
class ExigenceSDD : public Exigence {

private:
    std::vector<std::string> referencedSRS;  ///< Identifiants des exigences SRS référencées.
    bool developpe; ///< Indique si l'exigence a été développée.

public:
    /**
     * @brief Construit une exigence SDD.
     * @param id      Identifiant unique de l'exigence.
     * @param contenu Texte descriptif de l'exigence.
     */
    ExigenceSDD(const std::string& id, const std::string& contenu);

    /// @brief Retourne les identifiants SRS référencés par cette exigence.
    /// @return Référence constante sur le vecteur d'identifiants SRS.
    const std::vector<std::string>& getReferencedSRS() const;

    /// @brief Modifie la liste des SRS référencées.
    /// @param referencedSRS Vecteur d'identifiants SRS.
    void setReferencedSRS(const std::vector<std::string>& referencedSRS);
    bool getDeveloppe() const;
    void setDeveloppe(bool developpe);
};

#endif /* EXIGENCESDD_H_ */
