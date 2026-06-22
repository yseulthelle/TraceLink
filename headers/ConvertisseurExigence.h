/*
 * ConvertisseurExigence.h
 *
 *  Created on: 31 mars 2026
 */
#ifndef CONVERTISSEUREXIGENCE_H_
#define CONVERTISSEUREXIGENCE_H_

#include <vector>
#include <string>

#include "ExtracteurFichier.h"
#include "ExigenceSSS.h"
#include "ExigenceSRS.h"
#include "ExigenceSDD.h"

/**
 * @class ConvertisseurExigences
 * @brief Adaptateur qui transforme les structures brutes (Module 1)
 * en objets C++ typés (Module 2) prêts pour l'analyse de traçabilité.
 *
 * AJOUT : les préfixes SSS/SRS/SDD acceptent désormais plusieurs valeurs
 * séparées par des virgules (ex: "CLIENT, A, SSS").
 * Le préfixe global accepte aussi plusieurs valeurs (ex: "SPEC_, EX_, REQ_").
 */
class ConvertisseurExigences {
private:
    std::vector<ExigenceSSS> exigencesSSS;
    std::vector<ExigenceSRS> exigencesSRS;
    std::vector<ExigenceSDD> exigencesSDD;
    std::vector<std::string> ignorees;
    std::string motifRegex;

    // AJOUT : listes de préfixes configurables
    std::vector<std::string> prefixesSSS;  ///< Mots-clés SSS (ex: ["CLIENT", "A"])
    std::vector<std::string> prefixesSRS;  ///< Mots-clés SRS (ex: ["PROJET", "B"])
    std::vector<std::string> prefixesSDD;  ///< Mots-clés SDD (ex: ["SDD", "C"])

    /// @brief Découpe une chaîne par virgule et nettoie les espaces
    static std::vector<std::string> splitVirgule(const std::string& s);

    TypeExigence detecterType(const std::string& identifiant) const;
    bool estTypeConnu(const std::string& identifiant) const;

public:
    /**
     * @brief Constructeur avec préfixes multiples séparés par virgules.
     * @param motif   Regex pour matcher les identifiants
     * @param pSSS    Mots-clés SSS séparés par virgules (ex: "CLIENT, A")
     * @param pSRS    Mots-clés SRS séparés par virgules (ex: "PROJET, B")
     * @param pSDD    Mots-clés SDD séparés par virgules (ex: "SDD, C")
     */
    ConvertisseurExigences(const std::string& motif = "[A-Z][A-Z0-9_]+",
                           const std::string& pSSS  = "CLIENT",
                           const std::string& pSRS  = "PROJET",
                           const std::string& pSDD  = "SDD")
        : motifRegex(motif),
        prefixesSSS(splitVirgule(pSSS)),
        prefixesSRS(splitVirgule(pSRS)),
        prefixesSDD(splitVirgule(pSDD)) {}

    void convertir(const std::vector<ExigenceExtraite>& extraites);

    const std::vector<ExigenceSSS>& getSSS() const;
    const std::vector<ExigenceSRS>& getSRS() const;
    const std::vector<ExigenceSDD>& getSDD() const;
    const std::vector<std::string>& getIgnorees() const;
};

#endif /* CONVERTISSEUREXIGENCES_H_ */
