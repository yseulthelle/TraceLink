/*
 * ExigenceSRS.h
 *
 *  Created on: 9 mars 2026
 */

#ifndef EXIGENCESRS_H_
#define EXIGENCESRS_H_

#include "Exigence.h"
#include <vector>

/**
 * @class ExigenceSRS
 * @brief Exigence de niveau SRS (Software Requirements Specification).
 *
 * Hérite de @c Exigence. Contient :
 * - @c tracabilite : identifiants des exigences SSS qu'elle couvre,
 * - @c cibles      : composants ou modules ciblés par l'exigence,
 * - @c necessaireA : projets concernés par cette exigence.
 */
class ExigenceSRS : public Exigence {

private:
    std::vector<std::string> tracabilite;
    std::vector<std::string> cibles;
    std::vector<std::string> necessaireA;
    bool developpe;
    std::string etat;
    std::string methodeVerification;

public:
    ExigenceSRS(const std::string& id, const std::string& contenu);

    const std::vector<std::string>& getTracabilite() const;
    void setTracabilite(const std::vector<std::string>& tracabilite);

    const std::vector<std::string>& getCibles() const;
    void setCibles(const std::vector<std::string>& cibles);

    const std::vector<std::string>& getNecessaireA() const;
    void setNecessaireA(const std::vector<std::string>& necessaireA);

    bool getDeveloppe() const;
    void setDeveloppe(bool developpe);

    const std::string& getEtat() const;
    void setEtat(const std::string& etat);

    const std::string& getMethodeVerification() const;
    void setMethodeVerification(const std::string& m);
};

#endif /* EXIGENCESRS_H_ */



