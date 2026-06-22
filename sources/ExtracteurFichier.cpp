/**
 * @file ExtracteurFichier.cpp
 * @brief Module 1 : Import & Parsing
 */

#include "ExtracteurFichier.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <zip.h>
#include <set>
#include <pugixml.hpp>
#include <cstdint>
#include <cctype>
#include <sstream>
#include <algorithm>


/**
 * @brief Constructeur de l'extracteur de fichier.
 * @param chemin Chemin du fichier à analyser.
 * @param motif  Regex pour reconnaître les identifiants d'exigences.
 */
ExtracteurFichier::ExtracteurFichier(const std::string& chemin,
                                     const std::string& motif,
                                     const std::string& motifTracabilite)
    : cheminSource(chemin),
    motifRegex(motif),
    motifTracabiliteRegex(motifTracabilite.empty() ? motif : motifTracabilite),
    nbDetectees(0)
{
    if (chemin.empty()) {
        throw FileNotFoundException("(chemin vide)");
    }
}

/**
 * @brief Orchestre l'extraction complète :
 *        1. Vérifie l'existence du fichier
 *        2. Détecte l'extension
 *        3. Appelle le parseur adapté
 *        4. Retourne la liste structurée
 */
std::vector<ExigenceExtraite> ExtracteurFichier::extraire()
{
    std::vector<ExigenceExtraite> resultats;

    verifierExistence();

    std::string ext = detecterExtension();

    std::vector<std::pair<int, std::string>> lignes;

    if (ext == "csv") {
        lignes = lireCSV(ext);
    } else if (ext == "docx") {
        lignes = lireDocx();
    } else if (ext == "xlsx") {
        lignes = lireXlsx();
    } else if (ext == "doc") {
        lignes = lireDoc();
    } else if (ext == "xls") {
        lignes = lireXls();
    } else {
        throw FileFormatException("Format non supporté : '." + ext + "'. "
                                                                     "Formats acceptés : .docx, .csv, .xls");
    }

    if (ext == "docx" || ext == "doc") {
        resultats = parserBlocsDocx(lignes);
    } else if (ext == "xls") {
        // AJOUT : le .xls converti en .xlsx est traité comme un xlsx
        resultats = filtrerLignes(lignes, "xlsx");
    } else {
        resultats = filtrerLignes(lignes, ext);
    }

    nbDetectees = resultats.size();
    std::cout << "[Module extraction] " << nbDetectees
              << " exigence(s) détectée(s) dans « "
              << cheminSource << " »." << std::endl;

    return resultats;
}

/**
 * @brief Retourne le nombre d'exigences détectées lors de la dernière extraction.
 */
int ExtracteurFichier::getNombreExigencesDetectees() const
{
    return nbDetectees;
}

// ─── Méthodes privées ─────────────────────────────────────────────────────────

/**
 * @brief Retourne l'extension du fichier en minuscules.
 */
std::string ExtracteurFichier::detecterExtension() const
{
    size_t pos = cheminSource.rfind('.');
    if (pos == std::string::npos) return "";

    std::string ext = cheminSource.substr(pos + 1);
    for (size_t i = 0; i < ext.length(); ++i)
        ext[i] = std::tolower(static_cast<unsigned char>(ext[i]));
    return ext;
}

/**
 * @brief Vérifie si le fichier source est présent sur le disque.
 * @throw FileNotFoundException Si le fichier n'existe pas ou n'est pas accessible.
 */
void ExtracteurFichier::verifierExistence() const
{
    std::ifstream test(cheminSource);
    if (!test.is_open())
        throw FileNotFoundException(cheminSource);
}

/**
 * @brief Lit un fichier texte tabulaire (.csv ou .xls) ligne par ligne.
 *
 * Retourne chaque ligne brute (sans concaténation des colonnes)
 * pour que filtrerLignes() puisse traiter les colonnes séparément.
 * Le délimiteur (virgule ou point-virgule) est auto-détecté.
 * Les lignes entièrement vides sont ignorées.
 *
 * @param extension "csv" ou "xls"
 * @return Vecteur de (numéroLigne, ligne brute).
 */
std::vector<std::pair<int, std::string>> ExtracteurFichier::lireCSV(const std::string&) const
{
    std::ifstream fichier(cheminSource);
    if (!fichier.is_open())
        throw FileNotFoundException(cheminSource);

    std::vector<std::pair<int, std::string>> lignes;
    std::string ligneBrute;
    int numeroLigne = 0;

    while (std::getline(fichier, ligneBrute)) {
        numeroLigne++;

        // Nettoyer le \r final (fichiers Windows)
        if (!ligneBrute.empty() && ligneBrute.back() == '\r')
            ligneBrute.pop_back();

        // Ignorer les lignes vides
        if (ligneBrute.find_first_not_of(" \t\r\n") == std::string::npos)
            continue;

        lignes.push_back(std::make_pair(numeroLigne, ligneBrute));
    }

    fichier.close();
    return lignes;
}

/**
 * @brief Charge le dictionnaire des textes partagés d'Excel.
 * * Dans un .xlsx, les textes ne sont pas stockés directement dans les cellules
 * pour gagner de la place. Ils sont regroupés dans 'xl/sharedStrings.xml'.
 * * Cette méthode :
 * 1. Ouvre le fichier de dictionnaire dans l'archive ZIP.
 * 2. Lit tout le contenu XML.
 * 3. Stocke chaque texte trouvé dans un vecteur où l'indice correspond à l'ID Excel.
 * * @param archive Pointeur vers l'archive ZIP ouverte (libzip).
 * @return Un vecteur de chaînes de caractères servant de table de correspondance.
 */
std::vector<std::string> ExtracteurFichier::chargerSharedStrings(zip_t* archive) const {
    std::vector<std::string> catalogue;
    zip_file_t* f = zip_fopen(archive, "xl/sharedStrings.xml", 0);
    if (!f) return catalogue;

    std::string contenu;
    char buffer[4096];
    zip_int64_t n;
    while ((n = zip_fread(f, buffer, sizeof(buffer))) > 0)
        contenu.append(buffer, (size_t)n);
    zip_fclose(f);

    pugi::xml_document doc;
    doc.load_string(contenu.c_str());

    for (pugi::xpath_node t_xpath : doc.select_nodes("//t")) {
        catalogue.push_back(t_xpath.node().child_value());
    }
    return catalogue;
}

/**
 * @brief Extrait le texte brut d'un fichier .xlsx ligne par ligne.
 * * Un fichier .xlsx est une archive ZIP contenant xl/worksheets/sheet1.xml.
 * Cette méthode décompresse l'archive, charge le dictionnaire de textes,
 * et reconstruit les lignes du tableau.
 */
std::vector<std::pair<int, std::string>> ExtracteurFichier::lireXlsx() const {
    std::vector<std::pair<int, std::string>> lignes;
    int erreur = 0;
    // Ouverture du fichier XLSX comme une archive ZIP
    zip_t* archive = zip_open(cheminSource.c_str(), ZIP_RDONLY, &erreur);
    if (!archive) throw FileFormatException("Impossible d'ouvrir l'archive : " + cheminSource);

    // Chargement du dictionnaire "Shared Strings" (indispensable pour lire le texte Excel)
    std::vector<std::string> catalogueTextes = chargerSharedStrings(archive);

    // Extraction du fichier XML de la première feuille de calcul
    zip_file_t* xmlFichier = zip_fopen(archive, "xl/worksheets/sheet1.xml", 0);
    if (!xmlFichier) {
        zip_close(archive);
        throw FileFormatException("Fiche sheet1.xml introuvable");
    }

    // Lecture du flux de données XML vers une chaîne de caractères
    std::string xmlContenu;
    char buffer[4096];
    zip_int64_t nbLus;
    while ((nbLus = zip_fread(xmlFichier, buffer, sizeof(buffer))) > 0)
        xmlContenu.append(buffer, (size_t)nbLus);

    zip_fclose(xmlFichier);
    zip_close(archive); // On peut fermer l'archive, on a tout en mémoire

    // Analyse de la structure XML avec pugixml
    pugi::xml_document doc;
    doc.load_string(xmlContenu.c_str());

    int numLigne = 0;
    // On récupère toutes les balises <row> (les lignes du tableau)
    pugi::xpath_node_set rows = doc.select_nodes("//row");

    for (pugi::xpath_node row_xpath : rows) {
        pugi::xml_node row = row_xpath.node();
        std::string texteLigne;

        // Pour chaque ligne, on parcourt les cellules <c>
        int derniereColonne = 0; // AJOUT : pour gérer les cellules vides manquantes
        for (pugi::xpath_node cell_xpath : row.select_nodes("c")) {
            pugi::xml_node cell = cell_xpath.node();

            // détecter le numéro de colonne pour combler les vides
            std::string ref = cell.attribute("r").as_string(); // ex: "B2"
            int numCol = 0;
            for (char c : ref) {
                if (std::isalpha(c))
                    numCol = numCol * 26 + (std::toupper(c) - 'A' + 1);
                else break;
            }
            // AJOUT : combler les colonnes vides manquantes
            while (derniereColonne + 1 < numCol) {
                texteLigne += ";";
                derniereColonne++;
            }
            derniereColonne = numCol;

            // On vérifie si la cellule contient un texte partagé (type "s")
            std::string type = cell.attribute("t").as_string();
            std::string valeur = cell.child("v").text().as_string();

            if (type == "s" && !valeur.empty()) {
                // Si c'est un "Shared String", la valeur est un index (ex: "0")
                // On va chercher le vrai texte dans notre catalogue
                int index = std::stoi(valeur);
                if (index < (int)catalogueTextes.size()) {
                    texteLigne += catalogueTextes[index] + ";"; // Séparateur pour filtrerLignes()
                }
            } else if (type == "inlineStr") {
                // texte stocké directement dans la cellule (format openpyxl)
                pugi::xml_node is = cell.child("is");
                if (is) {
                    texteLigne += std::string(is.child("t").text().as_string()) + ";";
                } else {
                    texteLigne += ";";
                }
            } else if (!valeur.empty()) {
                // Sinon, c'est une valeur directe (nombre ou texte brut)
                texteLigne += valeur + ";";
            } else {
                // AJOUT : cellule présente mais vide
                texteLigne += ";";
            }
        }

        // Si la ligne n'est pas vide, on l'ajoute aux résultats
        if (!texteLigne.empty()) {
            numLigne++;
            lignes.push_back(std::make_pair(numLigne, texteLigne));
        }
    }
    return lignes;
}

/**
 * @brief Extrait le texte brut d'un fichier .docx paragraphe par paragraphe.
 *
 * Un fichier .docx est une archive ZIP contenant word/document.xml.
 * Ouvre l'archive avec libzip, parse le XML avec pugixml,
 * et retourne un vecteur de (numéroLigne, texte du paragraphe).
 *
 * @return Vecteur de (numéroLigne, texte du paragraphe).
 * @throw FileFormatException si le fichier n'est pas un ZIP valide.
 */
std::vector<std::pair<int, std::string>> ExtracteurFichier::lireDocx() const
{
    std::vector<std::pair<int, std::string>> lignes;

    // Ouverture de l'archive ZIP
    int erreur = 0;
    zip_t* archive = zip_open(cheminSource.c_str(), ZIP_RDONLY, &erreur);
    if (archive == nullptr)
        throw FileFormatException("Impossible d'ouvrir l'archive ZIP : " + cheminSource);

    // Localiser word/document.xml
    zip_file_t* xmlFichier = zip_fopen(archive, "word/document.xml", 0);
    if (xmlFichier == nullptr) {
        zip_close(archive);
        throw FileFormatException("word/document.xml introuvable dans : " + cheminSource);
    }

    // Lecture du contenu XML en mémoire
    std::string xmlContenu;
    char buffer[4096];
    zip_int64_t nbLus = 0;
    while ((nbLus = zip_fread(xmlFichier, buffer, sizeof(buffer))) > 0)
        xmlContenu.append(buffer, static_cast<size_t>(nbLus));

    zip_fclose(xmlFichier);
    zip_close(archive);

    if (xmlContenu.empty())
        throw FileFormatException("word/document.xml est vide dans : " + cheminSource);

    // Parser le XML avec pugixml
    pugi::xml_document doc;
    doc.load_string(xmlContenu.c_str());

    // Extraire le texte des paragraphes
    int numParagraphe = 0;
    for (int i = 0; i < (int)doc.select_nodes("//w:p").size(); i++) {
        pugi::xml_node para = doc.select_nodes("//w:p")[i].node();

        std::string texteParagraphe;
        for (int j = 0; j < (int)para.select_nodes(".//w:t").size(); j++) {
            pugi::xml_node t = para.select_nodes(".//w:t")[j].node();
            texteParagraphe += t.child_value();
        }

        if (texteParagraphe.find_first_not_of(" \t\r\n") == std::string::npos)
            continue;

        numParagraphe++;
        lignes.push_back(std::make_pair(numParagraphe, texteParagraphe));
    }

    return lignes;
}

/**
 * @brief Filtre les lignes CSV/XLS colonne par colonne.
 *
 * Pour chaque ligne :
 *   - Colonne 1 : identifiant principal (doit matcher la regex)
 *   - Colonne 2 : lien de traçabilité si c'est un identifiant valide
 *     (commence par EXIGENCE_ ou SDD_)
 *   - Colonnes suivantes : contenu textuel ignoré
 *
 * @param lignes  Résultat de lireCSV().
 * @param format  "csv" ou "xls".
 * @return Liste des exigences extraites avec métadonnées.
 */
std::vector<ExigenceExtraite> ExtracteurFichier::filtrerLignes(
    const std::vector<std::pair<int, std::string>>& lignes,
    const std::string& format) const
{
    std::vector<ExigenceExtraite> resultats;
    std::vector<std::string> dejaTrouves;
    std::string nomFichier = cheminSource;
    std::regex reg(motifRegex);

    // ── Extraire le nom court du fichier (sans chemin ni extension) ───────────
    std::string nomCourt = nomFichier;
    size_t posSlash = nomCourt.find_last_of("/\\");
    if (posSlash != std::string::npos) nomCourt = nomCourt.substr(posSlash + 1);
    size_t posPoint = nomCourt.rfind('.');
    if (posPoint != std::string::npos) nomCourt = nomCourt.substr(0, posPoint);

    // ── Lire l'en-tête pour détecter l'ordre des colonnes ────────────────────
    // Ex: "Exigence SYST_B_SSS, Exigence ALPHA_SRS" → col1=SSS, col2=SRS → inversé
    // Ex: "Exigence SRS, Exigence SSS, Statut, Contenu" → col1=SRS → normal
    bool formatInverseGlobal = false;
    if (!lignes.empty()) {
        std::string entete = lignes[0].second;
        entete.erase(0, entete.find_first_not_of(" \t\r\n\""));
        size_t last = entete.find_last_not_of(" \t\r\n\"");
        if (last != std::string::npos) entete = entete.substr(0, last + 1);

        std::string enteteUpper = entete;
        std::transform(enteteUpper.begin(), enteteUpper.end(),
                       enteteUpper.begin(), ::toupper);

        // Si SSS apparait avant SRS dans l'en-tête → format inversé
        // Si SSS apparait en premier dans l'en-tête → format inversé
        // (col1=SSS, col2=SRS) peu importe le nom exact de la colonne 2
        size_t posSSS = enteteUpper.find("SSS");
        size_t posSRS = enteteUpper.find("SRS");
        if (posSSS != std::string::npos) {
            if (posSRS == std::string::npos || posSSS < posSRS) {
                // SSS est en col1 → format inversé
                formatInverseGlobal = true;
            }
        }
    }

    // ── Détecter si c'est un fichier SDD à 1 colonne ─────────────────────────
    bool formatSDD1Colonne = false;
    int lignesTestees = 0;
    for (size_t i = 0; i < lignes.size(); i++) {
        std::string texte = lignes[i].second;
        if (texte.find_first_not_of(" \t\r\n") == std::string::npos) continue;

        char delim = (std::count(texte.begin(), texte.end(), ';') >=
                      std::count(texte.begin(), texte.end(), ',')) ? ';' : ',';

        std::vector<std::string> colonnes;
        std::istringstream flux(texte);
        std::string col;
        while (std::getline(flux, col, delim)) {
            col.erase(0, col.find_first_not_of(" \t\r\n\""));
            size_t last = col.find_last_not_of(" \t\r\n\"");
            if (last != std::string::npos) col = col.substr(0, last + 1);
            colonnes.push_back(col);
        }

        if (colonnes.size() == 1) {
            std::smatch m;
            if (std::regex_search(colonnes[0], m, reg)) {
                formatSDD1Colonne = true;
                break;
            }
            // 1 colonne mais pas de match → en-tête, on continue
            lignesTestees++;
            if (lignesTestees >= 2) break;
        } else {
            // Plus d'1 colonne → pas un SDD à 1 colonne
            break;
        }
    }

    // ── Cas SDD 1 colonne : 1 seul objet SDD avec toutes les refs SRS ────────
    if (formatSDD1Colonne) {
        ExigenceExtraite exSDD;
        exSDD.identifiant   = "SDD_" + nomCourt;
        exSDD.fichierSource = nomFichier;
        exSDD.formatSource  = format;
        exSDD.numeroLigne   = 1;

        for (size_t i = 0; i < lignes.size(); i++) {
            std::string texte = lignes[i].second;
            if (texte.find_first_not_of(" \t\r\n") == std::string::npos) continue;

            texte.erase(0, texte.find_first_not_of(" \t\r\n\""));
            size_t last = texte.find_last_not_of(" \t\r\n\"");
            if (last != std::string::npos) texte = texte.substr(0, last + 1);

            std::smatch m;
            if (std::regex_search(texte, m, reg)) {
                std::string idSRS = m.str();
                bool dejaDedans = false;
                for (const auto& t : exSDD.tracabilite)
                    if (t == idSRS) { dejaDedans = true; break; }
                if (!dejaDedans)
                    exSDD.tracabilite.push_back(idSRS);
            }
        }

        if (!exSDD.tracabilite.empty())
            resultats.push_back(exSDD);

        return resultats;
    }

    // ── Cas normal : traitement ligne par ligne ───────────────────────────────
    for (size_t i = 0; i < lignes.size(); i++) {
        int numLigne      = lignes[i].first;
        std::string texte = lignes[i].second;

        // Auto-détection du délimiteur : ';' ou ','
        char delim = (std::count(texte.begin(), texte.end(), ';') >=
                      std::count(texte.begin(), texte.end(), ',')) ? ';' : ',';

        // Découper en colonnes
        std::vector<std::string> colonnes;
        std::istringstream fluxCol(texte);
        std::string col;
        while (std::getline(fluxCol, col, delim)) {
            col.erase(0, col.find_first_not_of(" \t\r\n\""));
            size_t last = col.find_last_not_of(" \t\r\n\"");
            if (last != std::string::npos) col = col.substr(0, last + 1);
            colonnes.push_back(col);
        }

        if (colonnes.empty()) continue;

        // ── Détection du format via l'en-tête ─────────────────────────────────
        std::smatch match;
        std::string idPrincipal;
        bool formatInverse = false;

        if (formatInverseGlobal) {
            // En-tête indique col1=SSS, col2=SRS → ID principal en col2
            if (colonnes.size() >= 2 && std::regex_search(colonnes[1], match, reg)) {
                idPrincipal   = match.str();
                formatInverse = true;
            } else continue;
        } else {
            // Format normal : ID principal en col1
            if (std::regex_search(colonnes[0], match, reg)) {
                idPrincipal = match.str();
            } else continue;
        }

        // ── Vérifier doublon ──────────────────────────────────────────────────
        bool dejaVu = false;
        for (const auto& d : dejaTrouves)
            if (d == idPrincipal) { dejaVu = true; break; }
        if (dejaVu) continue;

        ExigenceExtraite ex;
        ex.identifiant   = idPrincipal;
        ex.contenuBrut   = texte;
        ex.fichierSource = nomFichier;
        ex.formatSource  = format;
        ex.numeroLigne   = numLigne;

        // ── Extraction de la tracabilité ──────────────────────────────────────
        if (formatInverse) {
            // Format inversé : col1 contient l'ID de tracabilité (SSS)
            std::regex regGenerique("[A-Z][A-Z0-9]*_[A-Z][A-Z0-9]*_[A-Z0-9_]+");
            std::smatch m;
            if (std::regex_search(colonnes[0], m, regGenerique)) {
                std::string idTrace = m.str();
                if (idTrace != idPrincipal)
                    ex.tracabilite.push_back(idTrace);
            }
        } else {
            // Cas normal : IDs dans colonnes suivantes, multi-IDs séparés par ;
            // On utilise une regex générique pour capturer TOUS les IDs
            // même ceux qui ne matchent pas le préfixe (liens brisés)
            std::regex regGenerique("[A-Z][A-Z0-9]*_[A-Z][A-Z0-9]*_[A-Z0-9_]+");
            for (size_t c = 1; c < colonnes.size(); c++) {
                std::string valCol = colonnes[c];
                if (valCol.empty()) continue;
                for (char& ch : valCol)
                    if (ch == ';') ch = ' ';
                std::string::const_iterator it = valCol.cbegin();
                std::smatch m;
                while (std::regex_search(it, valCol.cend(), m, regGenerique)) {
                    std::string idTrouve = m.str();
                    if (idTrouve != idPrincipal) {
                        bool dejaDansTrace = false;
                        for (const auto& t : ex.tracabilite)
                            if (t == idTrouve) { dejaDansTrace = true; break; }
                        if (!dejaDansTrace)
                            ex.tracabilite.push_back(idTrouve);
                    }
                    it = m.suffix().first;
                }
                // S'arrêter à la première colonne avec du contenu textuel
                // (col2 = statut, col3 = description → pas des IDs)
                break; // On ne lit QUE col1 pour la tracabilité
            }
        }

        // ── Colonnes supplémentaires : état + description ─────────────────────
        if (colonnes.size() >= 5) {
            ex.necessaireA.push_back(colonnes[2]);
            ex.developpe   = (colonnes[3] == "1");
            ex.description = colonnes[4];
        }
        else if (colonnes.size() == 4 && !formatInverse) {
            std::string statut = colonnes[2];
            std::transform(statut.begin(), statut.end(), statut.begin(), ::tolower);
            ex.etat        = colonnes[2];
            ex.developpe   = (statut.find("cours") != std::string::npos ||
                            statut.find("valid") != std::string::npos ||
                            statut == "1");
            ex.description = colonnes[3];
        }

        resultats.push_back(ex);
        dejaTrouves.push_back(idPrincipal);
    }

    return resultats;
}



/**
 * @brief Regroupe les paragraphes docx en blocs et extrait les métadonnées.
 *
 * Un bloc commence sur un identifiant (matché par regex) et se termine sur "#".
 * Extrait : tracabilite, cibles, necessaireA, alloueA, developpe.
 *
 * @param paragraphes Résultat de lireDocx().
 * @return Liste des exigences extraites avec métadonnées.
 */
std::vector<ExigenceExtraite> ExtracteurFichier::parserBlocsDocx(
    const std::vector<std::pair<int, std::string>>& paragraphes) const
{
    std::cout << "[DEBUG] parserBlocsDocx nb=" << paragraphes.size() << std::endl;
    if (!paragraphes.empty())
        std::cout << "[DEBUG] premier para=[" << paragraphes[0].second << "]" << std::endl;
    std::vector<ExigenceExtraite> resultats;
    std::set<std::string> dejaTrouves;
    std::regex reg(motifRegex);
    ExigenceExtraite exCourante;
    bool dansBloc = false;

    for (size_t i = 0; i < paragraphes.size(); i++) {
        const std::string& texte = paragraphes[i].second;

        // ── Détecter si la ligne est un ID seul sur sa ligne ──
        std::smatch match;
        std::string texteNettoye = texte;
        texteNettoye.erase(0, texteNettoye.find_first_not_of(" \t\r\n"));
        size_t last = texteNettoye.find_last_not_of(" \t\r\n");
        if (last != std::string::npos) texteNettoye = texteNettoye.substr(0, last + 1);
        bool estIdSeul = std::regex_search(texte, match, reg)
                         && texteNettoye == match.str();

        // ── Fin de bloc : # ou nouvel ID seul ──
        bool finParDiese = (texteNettoye == "#");
        bool finParNouvelId = dansBloc && estIdSeul
                              && match.str() != exCourante.identifiant;

        if (finParDiese || finParNouvelId) {
            if (dansBloc) {
                resultats.push_back(exCourante);
                dejaTrouves.insert(exCourante.identifiant);
                exCourante = ExigenceExtraite();
                dansBloc = false;
            }
            if (finParDiese) continue;
        }

        // ── Début de bloc : ID seul + non doublon ──
        if (!dansBloc && estIdSeul) {
            std::string idTrouve = match.str();
            if (dejaTrouves.count(idTrouve)) continue; // doublon → ignorer
            exCourante = ExigenceExtraite();
            exCourante.identifiant   = idTrouve;
            exCourante.contenuBrut   = texte;
            exCourante.fichierSource = cheminSource;
            exCourante.formatSource  = "docx";
            exCourante.numeroLigne   = paragraphes[i].first;
            dansBloc = true;
            continue;
        }

        // ── Contenu du bloc ──
        if (dansBloc) {

            if (texte.find("abilit") != std::string::npos
                && texte.find(":") != std::string::npos
                && texte.find("cessaire") == std::string::npos) {
                // Ligne Tracabilite : extraire TOUS les IDs avec regex générique
                // pour capturer aussi les liens brisés (IDs inexistants)
                std::regex regGenerique("[A-Z][A-Z0-9]*_[A-Z][A-Z0-9]*_[A-Z0-9_]+");
                std::string::const_iterator it(texte.cbegin());
                std::smatch m;
                while (std::regex_search(it, texte.cend(), m, regGenerique)) {
                    std::string id = m.str();
                    if (id != exCourante.identifiant)
                        exCourante.tracabilite.push_back(id);
                    it = m.suffix().first;
                }
            }
            else if (texte.find("Cible") != std::string::npos
                     && texte.find(":") != std::string::npos) {
                exCourante.cibles = decouper(texte.substr(texte.find(":") + 1));
            }
            else if (texte.find("cessaire") != std::string::npos
                     && texte.find(":") != std::string::npos) {
                exCourante.necessaireA = decouper(texte.substr(texte.find(":") + 1));
            }
            else if (texte.find("llou") != std::string::npos
                     && texte.find(":") != std::string::npos) {
                exCourante.alloueA = decouper(texte.substr(texte.find(":") + 1));
            }
            else if (texte.find("velopp") != std::string::npos
                     || texte.find("cours")  != std::string::npos
                     || texte.find("alid")   != std::string::npos) {
                exCourante.developpe = (texte.find("Non") == std::string::npos);
                if (texte.find("En cours") != std::string::npos)
                    exCourante.etat = "En cours";
                else if (texte.find("Non") != std::string::npos && texte.find("velopp") != std::string::npos)
                    exCourante.etat = "Non développé";
                else if (texte.find("velopp") != std::string::npos)
                    exCourante.etat = "Développé";
                else if (texte.find("nvalid") != std::string::npos)
                    exCourante.etat = "Invalide";
                else if (texte.find("alid") != std::string::npos)
                    exCourante.etat = "Valide";
                else
                    exCourante.etat = texte;
            }
            else if (texte == "Test" || texte == "Inspection" ||
                     texte == "Analyse" || texte == "Démonstration") {
                exCourante.methodeVerification = texte;
            }
            else if (!texte.empty() && exCourante.description.empty()) {
                if (texte.find(' ') != std::string::npos || texte.size() > 20)
                    exCourante.description = texte;
            }
        }
    }

    // Dernier bloc sans délimiteur final
    if (dansBloc)
        resultats.push_back(exCourante);

    return resultats;
}

/**
 * @brief Convertit un fichier .xls en .xlsx puis l'analyse.
 *
 * Un fichier .xls est un format binaire propriétaire (BIFF).
 * On utilise LibreOffice en ligne de commande pour le convertir
 * en .xlsx, puis on appelle lireXlsx() sur le résultat.
 *
 * @return Vecteur de (numéroLigne, texte du paragraphe).
 * @throw FileFormatException si la conversion échoue.
 */
std::vector<std::pair<int, std::string>> ExtracteurFichier::lireXls() const {
    // On récupère le dossier du fichier source
    std::string dossier = "./";
    size_t pos = cheminSource.rfind('/');
    if (pos == std::string::npos) pos = cheminSource.rfind('\\');
    if (pos != std::string::npos)
        dossier = cheminSource.substr(0, pos + 1);

    // Construction de la commande LibreOffice
    std::string commande = "\"\"C:\\Program Files\\LibreOffice\\program\\soffice.exe\" --headless --convert-to xlsx \""
                           + cheminSource
                           + "\" --outdir \""
                           + dossier
                           + "\"\"";
    std::cout << "[Module extraction] Conversion .xls → .xlsx en cours...\n";

    int codeRetour = std::system(commande.c_str());
    if (codeRetour != 0) {
        throw FileFormatException(
            "Conversion .xls échouée : " + cheminSource
            );
    }

    // Construire le chemin du fichier .xlsx converti
    std::string cheminXlsx = cheminSource;
    size_t posPoint = cheminXlsx.rfind('.');
    if (posPoint != std::string::npos)
        cheminXlsx = cheminXlsx.substr(0, posPoint) + ".xlsx";

    std::cout << "[Module extraction] Fichier converti : " << cheminXlsx << "\n";

    // Vérifier que le fichier converti existe
    std::ifstream test(cheminXlsx);
    if (!test.is_open()) {
        throw FileFormatException(
            "Fichier .xlsx converti introuvable : " + cheminXlsx
            );
    }
    test.close();

    // Lire le .xlsx converti
    ExtracteurFichier extracteurTemp(cheminXlsx, "");
    return extracteurTemp.lireXlsx();
}
/**
 * @brief Découpe une chaîne "val1, val2, val3" en vecteur de tokens nettoyés.
 * @param ligne La chaîne à découper.
 * @return Vecteur de tokens.
 */


std::vector<std::string> ExtracteurFichier::decouper(const std::string& ligne) const
{
    std::vector<std::string> tokens;
    std::istringstream flux(ligne);
    std::string token;
    while (std::getline(flux, token, ',')) {
        size_t debut = token.find_first_not_of(" \t\r\n");
        size_t fin   = token.find_last_not_of(" \t\r\n");
        if (debut != std::string::npos)
            tokens.push_back(token.substr(debut, fin - debut + 1));
    }
    return tokens;
}

/**
 * @brief Convertit un fichier .doc en .docx puis l'analyse.
 *
 * Un fichier .doc est un format binaire propriétaire (BIFF).
 * Il ne peut pas être lu directement comme un ZIP.
 * On utilise LibreOffice en ligne de commande pour le convertir
 * en .docx, puis on appelle lireDocx() sur le résultat.
 *
 * Prérequis : LibreOffice installé sur le système
 *   macOS  : brew install libreoffice
 *   Linux  : sudo apt install libreoffice
 *   Windows : télécharger sur libreoffice.org
 *
 * @return Vecteur de (numéroLigne, texte du paragraphe).
 * @throw FileFormatException si la conversion échoue.
 */
std::vector<std::pair<int, std::string>> ExtracteurFichier::lireDoc() const{
    // Construire la commande de conversion
    // LibreOffice peut convertir .doc en .docx en ligne de commande
    // --headless = sans interface graphique
    // --convert-to docx = format de sortie
    // --outdir = dossier de sortie (même dossier que le fichier source)

    // On récupère le dossier du fichier source
    std::string dossier = "./"; // dossier courant par défaut
    size_t pos = cheminSource.rfind('/');
    if (pos == std::string::npos) pos = cheminSource.rfind('\\');
    if (pos != std::string::npos) {
        dossier = cheminSource.substr(0, pos + 1);
    }

    // Construction de la commande LibreOffice
    std::string commande = "\"\"C:\\Program Files\\LibreOffice\\program\\soffice.exe\" --headless --convert-to docx \""
                           + cheminSource
                           + "\" --outdir \""
                           + dossier
                           + "\"\"";
    std::cout << "[Module extraction] Conversion .doc → .docx en cours...\n";

    // Lancement de la conversion
    int codeRetour = std::system(commande.c_str());

    if (codeRetour != 0) {
        throw FileFormatException(
            "Conversion .doc échouée. "
            "Vérifiez que LibreOffice est installé : " + cheminSource
            );
    }

    // Construire le chemin du fichier .docx converti
    // LibreOffice crée un fichier avec le même nom mais en .docx
    // "mon_fichier.doc" → "mon_fichier.docx"

    std::string cheminDocx = cheminSource;
    size_t posPoint = cheminDocx.rfind('.');
    if (posPoint != std::string::npos) {
        cheminDocx = cheminDocx.substr(0, posPoint) + ".docx";
    }

    std::cout << "[Module extraction] Fichier converti : " << cheminDocx << "\n";

    // Vérifier que le fichier converti existe
    std::ifstream test(cheminDocx);
    if (!test.is_open()) {
        throw FileFormatException(
            "Fichier .docx converti introuvable : " + cheminDocx
            );
    }
    test.close();

    // Lire le .docx converti avec lireDocx()
    // On crée un extracteur temporaire pointant vers le .docx
    ExtracteurFichier extracteurTemp(cheminDocx, "");
    return extracteurTemp.lireDocx();
}
