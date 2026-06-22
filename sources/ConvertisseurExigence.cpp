/*
 * ConvertisseurExigence.cpp
 *
 *  Created on: 31 mars 2026
 */
#include "ConvertisseurExigence.h"
#include <algorithm>
#include <cctype>
#include <sstream>

static std::string enMajuscules(const std::string& s) {
    std::string res = s;
    std::transform(res.begin(), res.end(), res.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return res;
}

// ─── splitVirgule ─────────────────────────────────────────────────────────────
/**
 * @brief Découpe une chaîne par virgule et nettoie les espaces.
 * Ex: "CLIENT, A, SSS" → ["CLIENT", "A", "SSS"]
 */
std::vector<std::string> ConvertisseurExigences::splitVirgule(const std::string& s) {
    std::vector<std::string> result;
    std::istringstream flux(s);
    std::string token;
    while (std::getline(flux, token, ',')) {
        // Nettoyer espaces
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        size_t last = token.find_last_not_of(" \t\r\n");
        if (last != std::string::npos) token = token.substr(0, last + 1);
        if (!token.empty()) result.push_back(token);
    }
    return result;
}

// ─── estTypeConnu ─────────────────────────────────────────────────────────────
/**
 * @brief Vérifie si l'identifiant contient au moins un mot-clé SSS, SRS ou SDD.
 * AJOUT : supporte plusieurs préfixes par niveau.
 */
bool ConvertisseurExigences::estTypeConnu(const std::string& identifiant) const {
    std::string upper = enMajuscules(identifiant);
    for (const auto& p : prefixesSSS)
        if (!p.empty() && upper.find(enMajuscules(p)) != std::string::npos) return true;
    for (const auto& p : prefixesSRS)
        if (!p.empty() && upper.find(enMajuscules(p)) != std::string::npos) return true;
    for (const auto& p : prefixesSDD)
        if (!p.empty() && upper.find(enMajuscules(p)) != std::string::npos) return true;
    return false;
}

// ─── detecterType ─────────────────────────────────────────────────────────────
/**
 * @brief Détecte le type SSS/SRS/SDD depuis l'identifiant.
 * AJOUT : supporte plusieurs préfixes par niveau.
 */
TypeExigence ConvertisseurExigences::detecterType(const std::string& identifiant) const {
    std::string upper = enMajuscules(identifiant);
    for (const auto& p : prefixesSSS)
        if (!p.empty() && upper.find(enMajuscules(p)) != std::string::npos) return SSS;
    for (const auto& p : prefixesSRS)
        if (!p.empty() && upper.find(enMajuscules(p)) != std::string::npos) return SRS;
    for (const auto& p : prefixesSDD)
        if (!p.empty() && upper.find(enMajuscules(p)) != std::string::npos) return SDD;
    return SSS;
}

// ─── convertir ───────────────────────────────────────────────────────────────
void ConvertisseurExigences::convertir(const std::vector<ExigenceExtraite>& extraites) {
    exigencesSSS.clear();
    exigencesSRS.clear();
    exigencesSDD.clear();
    ignorees.clear();

    for (const ExigenceExtraite& ex : extraites) {
        if (!estTypeConnu(ex.identifiant)) {
            ignorees.push_back(ex.identifiant);
            continue;
        }

        TypeExigence type = detecterType(ex.identifiant);

        switch (type) {
        case SSS: {
            ExigenceSSS obj(ex.identifiant, ex.description);
            obj.setAlloueA(ex.alloueA);
            obj.setDeveloppe(ex.developpe);
            obj.setEtat(ex.etat);
            obj.setMethodeVerification(ex.methodeVerification);
            obj.setFichierSource(ex.fichierSource);
            exigencesSSS.push_back(obj);
            break;
        }
        case SRS: {
            ExigenceSRS obj(ex.identifiant, ex.description);
            obj.setTracabilite(ex.tracabilite);
            obj.setCibles(ex.cibles);
            obj.setNecessaireA(ex.necessaireA);
            obj.setDeveloppe(ex.developpe);
            obj.setEtat(ex.etat);
            obj.setMethodeVerification(ex.methodeVerification);
            obj.setFichierSource(ex.fichierSource);
            exigencesSRS.push_back(obj);
            break;
        }
        case SDD: {
            ExigenceSDD obj(ex.identifiant, ex.description);
            obj.setReferencedSRS(ex.tracabilite);
            obj.setDeveloppe(ex.developpe);
            obj.setFichierSource(ex.fichierSource);
            exigencesSDD.push_back(obj);
            break;
        }
        }
    }
}

// ─── Accesseurs ──────────────────────────────────────────────────────────────
const std::vector<ExigenceSSS>& ConvertisseurExigences::getSSS() const { return exigencesSSS; }
const std::vector<ExigenceSRS>& ConvertisseurExigences::getSRS() const { return exigencesSRS; }
const std::vector<ExigenceSDD>& ConvertisseurExigences::getSDD() const { return exigencesSDD; }
const std::vector<std::string>& ConvertisseurExigences::getIgnorees() const { return ignorees; }
