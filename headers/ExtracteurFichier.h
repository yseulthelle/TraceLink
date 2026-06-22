/**@file ExtracteurFichier.h
 * @brief Module 1 : Import & Parsing
 * Convertit un fichier physique (Word/CSV/XLS) en liste d'exigences exploitables par le Moteur de Traçabilité (Module 2).
 * Entrées  : chemin du fichier + motif regex
 * Sorties  : liste d'objets Exigence structurés
 */

#ifndef EXTRACTEURFICHIER_H
#define EXTRACTEURFICHIER_H
#include <string>
#include <vector>
#include <regex>
#include <stdexcept>
#include <zip.h>

/**
 * @struct ExigenceExtraite
 * @brief Structure représentant une exigence extraite d'un fichier source.
 *
 * Champs de base :
 *   identifiant   : identifiant extrait par la regex (ex: EXIGENCE_CLIENT_A_F01)
 *   contenuBrut   : texte descriptif de l'exigence
 *   fichierSource : chemin du fichier d'origine
 *   formatSource  : extension détectée (csv, xls, docx)
 *   numeroLigne   : numéro de ligne dans le fichier source
 *
 * Métadonnées de traçabilité :
 *   tracabilite   : IDs SSS couverts par cette SRS  (colonne 2 CSV / ligne "Traçabilité :" docx)
 *   cibles        : cibles logicielles/matérielles   (ligne "Cible :" docx)
 *   necessaireA   : projets/clients concernés        (ligne "Nécessaire à :" docx)
 *   alloueA       : projets auxquels la SSS est allouée (ligne "Alloué à :" docx)
 *   developpe     : true si "Développé" figure dans le bloc docx
 */
struct ExigenceExtraite {
    std::string identifiant;
    std::string contenuBrut;
    std::string fichierSource;
    std::string formatSource;
    int         numeroLigne;

    std::vector<std::string> tracabilite;
    std::vector<std::string> cibles;
    std::vector<std::string> necessaireA;
    std::vector<std::string> alloueA;
    bool        developpe = false;
    std::string etat;
    std::string methodeVerification;
    std::string description;

    std::string detecterExtension() const;
};

/** Classe principale : ExtracteurFichier */
class ExtracteurFichier {
private:
    std::string cheminSource;
    std::regex motifRegex;
    std::regex motifTracabiliteRegex;
    std::vector<std::string> listeIdentifiants;
    int nbDetectees;

    /** Vérifie que le fichier existe et est lisible. */
    void verifierExistence() const;

    /**
     * @brief Lit un fichier CSV/XLS ligne par ligne.
     *
     * Retourne un vecteur de (numéroLigne, ligne brute) sans filtrage.
     * Le délimiteur (virgule ou point-virgule) est auto-détecté.
     * Les colonnes sont séparées mais la ligne est retournée brute
     * pour que filtrerLignes() puisse la traiter colonne par colonne.
     *
     * @param extension "csv" ou "xls"
     * @return Vecteur de (numéroLigne, contenu brut de la ligne).
     */
    std::vector<std::pair<int, std::string>> lireCSV(const std::string& extension) const;

    /**
     * @brief Extrait le texte brut d'un fichier .docx paragraphe par paragraphe.
     *
     * Décompresse l'archive ZIP, lit word/document.xml avec tinyxml2,
     * et retourne un vecteur de (numéroLigne, texte du paragraphe).
     *
     * @return Vecteur de (numéroLigne, texte du paragraphe).
     * @throw FileFormatException si le fichier n'est pas un ZIP valide.
     */
    std::vector<std::pair<int, std::string>> lireDocx() const;

    std::vector<std::pair<int, std::string>> lireXlsx() const;

    std::vector<std::string> chargerSharedStrings(zip_t* archive) const;



    /**
     * @brief Filtre les lignes CSV/XLS colonne par colonne.
     *
     * Pour chaque ligne :
     *   - Colonne 1 : identifiant (doit matcher la regex)
     *   - Colonne 2 : lien de traçabilité si c'est un identifiant, sinon contenu
     *   - Colonnes suivantes : contenu textuel
     *
     * Cette méthode est partageable avec le DOCX pour un filtrage uniforme.
     *
     * @param lignes  Résultat de lireCSV().
     * @param format  "csv" ou "xls".
     * @return Liste des exigences extraites avec métadonnées.
     */
    std::vector<ExigenceExtraite> filtrerLignes(
        const std::vector<std::pair<int, std::string>>& lignes,
        const std::string& format) const;

    /**
     * @brief Regroupe les paragraphes docx en blocs et extrait les métadonnées.
     *
     * Un bloc commence sur un identifiant (matché par regex) et se termine sur "#".
     * Extrait : tracabilite, cibles, necessaireA, alloueA, developpe.
     *
     * @param paragraphes Résultat de lireDocx().
     * @return Liste des exigences extraites avec métadonnées.
     */
    std::vector<ExigenceExtraite> parserBlocsDocx(
        const std::vector<std::pair<int, std::string>>& paragraphes) const;

    /**
     * @brief Découpe une chaîne "val1, val2, val3" en vecteur de tokens nettoyés.
     */
    std::vector<std::string> decouper(const std::string& ligne) const;

    /**  Convertit un fichier .doc en .docx puis l'analyse. */
    std::vector<std::pair<int, std::string>> lireDoc() const;
    /**  Convertit un fichier .xls en .xlsx puis l'analyse. */
    std::vector<std::pair<int, std::string>> lireXls() const;

public:
    ExtracteurFichier(const std::string& chemin, const std::string& motif, const std::string& motifTracabilite = "");
    std::vector<ExigenceExtraite> extraire();
    int getNombreExigencesDetectees() const;
    /** Retourne l'extension du fichier en minuscules. */
    std::string detecterExtension() const;
};

/** Levée quand le fichier est introuvable ou inaccessible */
class FileNotFoundException : public std::runtime_error {
public:
    explicit FileNotFoundException(const std::string& chemin)
        : std::runtime_error("Fichier introuvable : " + chemin) {}
};

/** Levée quand le format du fichier n'est pas supporté */
class FileFormatException : public std::runtime_error {
public:
    explicit FileFormatException(const std::string& detail)
        : std::runtime_error("Format non supporté : " + detail) {}
};

#endif
