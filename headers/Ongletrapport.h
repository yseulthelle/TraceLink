/**
 * @file OngletRapport.h
 * @brief Declaration de la classe OngletRapport.
 * @date 26/04/2026
 * @version 2.1 - Export HTML interactif : tableau 3 colonnes (Exigence, Statut, Couverte par)
 */

#ifndef ONGLETRAPPORT_H
#define ONGLETRAPPORT_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QProgressBar>
#include <QTabWidget>
#include <QMenu>
#include <QToolButton>

#include <QPrinter>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>
#include <QDateTime>

#include "RapportTracabilite.h"
#include "GestionnaireFiltresGUI.h"
#include "ExigenceSRS.h"
#include "ExigenceSDD.h"

/**
 * @class OngletRapport
 * @brief Onglet Qt qui affiche le rapport de tracabilite complet produit par le Module 2.
 *
 * Contenu de l'onglet :
 *   - Taux de couverture SSS->SRS, SRS->SDD et global avec barres de progression
 *   - Badges de resume : couvertes, non couvertes, liens brises, orphelines
 *   - Matrice SSS x SRS (3 colonnes : Exigence | Statut | Couverte par)
 *   - Matrice SRS x SDD (3 colonnes : Exigence | Statut | Couverte par)
 *   - Panneau de detail au clic sur une exigence
 *   - Panneau de liens brises (cadre orange, clic sur badge)
 *   - Panneau d'orphelines (cadre violet, clic sur badge)
 *   - Export CSV, HTML interactif, PDF
 *   - Panneau de filtres lateral
 *
 * @author Lamia Arrahmane (interface, matrices, filtres, CSV, PDF)
 * @author Amel Kheloul (export HTML interactif v2.1)
 */
class OngletRapport : public QWidget
{
    Q_OBJECT

public:

    /**
     * @brief Constructeur : cree l'onglet et initialise tous ses widgets.
     * @param parent Le widget parent (peut etre nullptr).
     */
    explicit OngletRapport(QWidget *parent = nullptr);

    /**
     * @brief Affiche un rapport sans donnees SRS/SDD detaillees.
     *        Appelle afficherRapportComplet avec des vecteurs vides.
     * @param rapport Le rapport produit par le moteur de tracabilite.
     */
    void afficherRapport(const RapportTracabilite& rapport);

    /**
     * @brief Affiche le rapport complet avec toutes les matrices et panneaux.
     *
     * Met a jour les taux, les matrices SSS x SRS et SRS x SDD,
     * les badges de resume, les panneaux liens brises et orphelines.
     *
     * @param rapport    Le rapport produit par MoteurTracabilite.
     * @param srs        Liste des exigences SRS analysees.
     * @param sdd        Liste des elements de conception SDD analyses.
     * @param orphelines Liste des identifiants d'exigences orphelines (sans parent SSS).
     */
    void afficherRapportComplet(const RapportTracabilite& rapport,
                                const std::vector<ExigenceSRS>& srs,
                                const std::vector<ExigenceSDD>& sdd,
                                const std::vector<std::string>& orphelines);

private slots:

    /// @brief Exporte le rapport au format CSV avec taux, matrices et liens brises.
    void exporterCSV();

    /**
     * @brief Exporte le rapport au format HTML interactif.
     *
     * Genere une page HTML avec :
     *   - Tableau 3 colonnes : Exigence | Statut | Couverte par
     *   - Recherche en temps reel par identifiant
     *   - Filtres : Tout / Couvert / Non couvert / Lien brise / Orpheline
     *   - Onglets SSS->SRS et SRS->SDD
     *   - Barres de progression animees
     *
     * @author Amel Kheloul
     */
    void exporterHTML();

    /// @brief Exporte le rapport au format PDF via QTextDocument et QPrinter.
    void exporterPDF();

    /// @brief Affiche ou cache le panneau de filtres lateral.
    void toggleFiltres();

public slots:

    /**
     * @brief Applique les criteres de filtre sur les deux matrices.
     *        Cache les lignes qui ne correspondent pas au filtre.
     * @param filtre L'objet filtre contenant les criteres : identifiant, statut, etat, orpheline.
     */
    void appliquerFiltre(const FiltreGUI& filtre);

    /**
     * @brief Intercepte les evenements souris sur les badges liens brises et orphelines.
     *        Un clic sur ces badges affiche ou cache le panneau correspondant.
     * @param obj   Le widget sur lequel l'evenement s'est produit.
     * @param event L'evenement intercepte.
     * @return true si l'evenement a ete traite ici, false sinon.
     */
    bool eventFilter(QObject* obj, QEvent* event) override;

private:

    //  Taux de couverture

    QLabel       *labelTauxSSS_SRS;  ///< Label "Taux SSS->SRS : X %" a gauche de la barre
    QLabel       *labelTauxSRS_SDD;  ///< Label "Taux SRS->SDD : X %" a gauche de la barre
    QLabel       *labelTauxGlobal;   ///< Label "Taux global : X %" a gauche de la barre
    QProgressBar *barreSSS_SRS;      ///< Barre de progression pour le taux SSS->SRS
    QProgressBar *barreSRS_SDD;      ///< Barre de progression pour le taux SRS->SDD
    QProgressBar *barreGlobal;       ///< Barre de progression pour le taux global

    //  Badges resume

    QLabel *labelBadgeCouverts;      ///< Badge vert : nombre d'exigences couvertes (SSS + SRS)
    QLabel *labelBadgeNonCouverts;   ///< Badge rouge : nombre d'exigences non couvertes
    QLabel *labelBadgeLiensBrises;   ///< Badge orange cliquable : nombre de liens brises
    QLabel *labelBadgeOrphelines;    ///< Badge violet cliquable : nombre d'exigences orphelines

    //  Matrices de tracabilite

    /// @brief Matrice SSS x SRS — 3 colonnes : Exigence | Statut | Couverte par (SRS)
    QTableWidget *tableMatriceSSS_SRS;

    /// @brief Matrice SRS x SDD — 3 colonnes : Exigence | Statut | Couverte par (SDD)
    QTableWidget *tableMatriceSRS_SDD;

    //  Panneau detail au clic

    QFrame  *panneauDetail;          ///< Cadre qui s'affiche au clic sur une ligne de matrice
    QLabel  *labelDetailId;          ///< Identifiant de l'exigence selectionnee
    QLabel  *labelDetailType;        ///< Badge type : "SSS" (bleu) ou "SRS" (orange)
    QLabel  *labelDetailStatut;      ///< Badge statut colore selon la couverture
    QLabel  *labelDetailLiens;       ///< Liste des identifiants des exigences liees
    QLabel  *labelDetailFichier;     ///< Chemin du fichier source (italic gris)
    QLabel  *labelDetailDescription; ///< Contenu textuel de l'exigence
    QLabel  *labelDetailEtat;        ///< Etat de developpement : Developpe / Non developpe

    //  Bouton et menu d'export

    QToolButton *boutonExport;  ///< Bouton vert "Exporter" avec menu deroulant
    QMenu       *menuExport;    ///< Menu : CSV, HTML interactif, PDF

    //  Panneau de filtres lateral

    QFrame                 *panneauFiltres;       ///< Cadre lateral qui se glisse depuis la droite
    QPushButton            *boutonFiltres;        ///< Bouton "Filtres" ouvre/ferme le panneau
    GestionnaireFiltresGUI *gestionnaireFiltres;  ///< Widget interne avec les champs de filtre

    // ── Donnees stockees pour exports et filtrage ─────────────────────────────

    RapportTracabilite       rapportCourant;      ///< Copie du dernier rapport charge
    std::vector<ExigenceSRS> srsAnalyses;         ///< Exigences SRS du dernier rapport
    std::vector<ExigenceSDD> sddAnalyses;         ///< Elements SDD du dernier rapport
    std::vector<std::string> orphelinesAnalyses;  ///< IDs des exigences orphelines

    //  Panneau liens brises

    QFrame      *panneauLiensBrises;  ///< Cadre orange listant les liens brises
    QWidget     *listeLiensBrises;    ///< Conteneur interne des lignes
    QVBoxLayout *layoutListeLB;       ///< Layout vertical des lignes de liens brises

    //  Panneau orphelines

    QFrame      *panneauOrphelines;   ///< Cadre violet listant les exigences orphelines
    QWidget     *listeOrphelines;     ///< Conteneur interne des lignes
    QVBoxLayout *layoutListeOrph;     ///< Layout vertical des lignes d'orphelines

    //  Methodes privees

    /**
     * @brief Construit et place tous les widgets dans l'onglet.
     *        Appelee une seule fois dans le constructeur.
     */
    void initialiserUI();

    /**
     * @brief Cree une barre de progression stylisee avec la couleur donnee.
     * @param couleur Couleur de remplissage en hexadecimal (ex: "#4A90D9").
     * @return Pointeur vers la barre creee.
     */
    QProgressBar* creerBarre(const QString& couleur);

    /**
     * @brief Applique un style visuel coherent a un tableau Qt.
     * @param table Le QTableWidget a styliser.
     */
    void styliserTableau(QTableWidget* table);
};

#endif // ONGLETRAPPORT_H