/*
 * ExigenceSSS.h
 *
 *  Created on: 9 mars 2026
 */

#ifndef EXIGENCESSS_H_
#define EXIGENCESSS_H_

#include "Exigence.h"
#include <vector>

/**
 * @class ExigenceSSS
 * @brief Exigence de niveau SSS (System/Subsystem Specification).
 *
 * Hérite de @c Exigence. Porte deux attributs spécifiques :
 * - un indicateur de développement (@c developpe),
 * - la liste des projets auxquels elle est allouée (@c alloueA).
 */
class ExigenceSSS : public Exigence {

private:
    bool developpe;
    std::string etat;
    std::string methodeVerification;
    std::vector<std::string> alloueA;

public:
    ExigenceSSS(const std::string& id, const std::string& contenu);

    bool getDeveloppe() const;
    void setDeveloppe(bool developpe);

    const std::string& getEtat() const;
    void setEtat(const std::string& etat);

    const std::string& getMethodeVerification() const;
    void setMethodeVerification(const std::string& m);

    const std::vector<std::string>& getAlloueA() const;
    void setAlloueA(const std::vector<std::string>& alloueA);
};
#endif /* EXIGENCESSS_H_ */


