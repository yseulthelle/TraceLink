/**
 * \file OngletRapport.cpp
 * \brief Implementation de la classe OngletRapport.
 * \date 02/05/2026
 */

#include "OngletRapport.h"

#include <QHeaderView>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QEvent>
#include <algorithm>

#include <QPrinter>
#include <QTextDocument>
#include <QDialog>
#include <QCheckBox>


/**
 * \brief Constructeur de l'onglet rapport.
 *
 * Cree l'onglet et appelle initialiserUI pour construire tous les widgets.
 *
 * \param parent Le widget parent Qt.
 */
OngletRapport::OngletRapport(QWidget *parent)
    : QWidget(parent)
{
    initialiserUI();
}


/**
 * \brief Cree une barre de progression stylisee avec la couleur donnee.
 *
 * La barre va de 0 a 100, avec un style arrondi et la couleur passee en parametre.
 *
 * \param couleur Couleur de remplissage en hexadecimal, ex "#4A90D9".
 * \return Un pointeur vers la barre de progression creee.
 */
QProgressBar* OngletRapport::creerBarre(const QString& couleur)
{
    QProgressBar* barre = new QProgressBar(this);
    barre->setRange(0, 100);
    barre->setValue(0);
    barre->setTextVisible(true);
    barre->setFixedHeight(22);
    barre->setAlignment(Qt::AlignCenter);
    barre->setStyleSheet(QString(R"(
        QProgressBar {
            background-color: #D6EAF8;
            border-radius: 11px;
            color: #1A1A2E;
            font-size: 11px;
            font-weight: bold;
            text-align: center;
        }
        QProgressBar::chunk {
            background-color: %1;
            border-radius: 11px;
        }
    )").arg(couleur));
    return barre;
}

/**
 * \brief Applique un style visuel coherent a un QTableWidget.
 *
 * Toutes les matrices de l'onglet utilisent ce style commun :
 * fond blanc, en-tetes bleus, lignes non editables.
 *
 * \param table Le tableau Qt a styliser.
 */
void OngletRapport::styliserTableau(QTableWidget* table)
{
    table->setStyleSheet(R"(
        QTableWidget {
            background-color: #FFFFFF;
            color: #333333;
            gridline-color: #DDDDDD;
            border: none;
            font-size: 11px;
        }
        QTableWidget::item { padding: 4px; }
        QHeaderView::section {
            background-color: #EBF5FB;
            color: #1A5276;
            font-weight: bold;
            padding: 6px;
            border: 1px solid #AED6F1;
            font-size: 11px;
        }
        QTableWidget::item:selected {
            background-color: #AED6F1;
            color: #1A5276;
        }
    )");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(false);
    table->verticalHeader()->setDefaultSectionSize(28);
    table->horizontalHeader()->setStretchLastSection(false);
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->verticalHeader()->setStyleSheet(R"(
        QHeaderView::section {
            background-color: #EBF5FB;
            color: #1A5276;
            font-weight: bold;
            padding: 4px;
            border: 1px solid #AED6F1;
            font-size: 10px;
        }
    )");
}


/**
 * \brief Construit et place tous les widgets de l'onglet rapport.
 *
 * Appelee une seule fois dans le constructeur.
 *
 * Structure de l'interface :
 *   Layout global horizontal : contenu principal | panneau filtres lateral
 *   Contenu principal :
 *     En-tete : titre + 4 badges + boutons Export/Filtres
 *     Section taux : 3 barres de progression (SSS->SRS, SRS->SDD, Global)
 *     Boutons zoom : Zoom+, Zoom-, Reset
 *     Onglets matrices : SSS->SRS | SRS->SDD (chacun 3 colonnes)
 *     Panneau liens brises (orange, visible si liens brises detectes)
 *     Panneau orphelines (violet, visible si orphelines detectees)
 *     Panneau detail (blanc, visible au clic sur une ligne de matrice)
 *   Panneau filtres : GestionnaireFiltresGUI lateral droit, cache par defaut
 */
void OngletRapport::initialiserUI()
{
    QHBoxLayout *layoutGlobal = new QHBoxLayout(this);
    layoutGlobal->setContentsMargins(0, 0, 0, 0);
    layoutGlobal->setSpacing(0);

    QWidget *contenu = new QWidget(this);
    contenu->setStyleSheet("background-color: #EBF5FB;");
    QVBoxLayout *layoutPrincipal = new QVBoxLayout(contenu);
    layoutPrincipal->setContentsMargins(12, 12, 12, 12);
    layoutPrincipal->setSpacing(10);

    // En-tete : titre + badges + boutons
    QHBoxLayout *layoutEntete = new QHBoxLayout();

    QLabel *titre = new QLabel("Rapport de tracabilite", this);
    titre->setStyleSheet("color: #1A5276; font-size: 14px; font-weight: bold;");
    layoutEntete->addWidget(titre);
    layoutEntete->addStretch();

    // Badge vert : nombre d'exigences couvertes (SSS + SRS)
    labelBadgeCouverts = new QLabel("0 Couvert", this);
    labelBadgeCouverts->setStyleSheet(
        "background-color:#27AE60; color:white; border-radius:12px;"
        "padding:4px 10px; font-size:11px; font-weight:bold;");
    labelBadgeCouverts->setToolTip("Exigence reliee a au moins une exigence du niveau inferieur.");

    // Badge rouge : nombre d'exigences non couvertes
    labelBadgeNonCouverts = new QLabel("0 Non couvert", this);
    labelBadgeNonCouverts->setStyleSheet(
        "background-color:#E74C3C; color:white; border-radius:12px;"
        "padding:4px 10px; font-size:11px; font-weight:bold;");
    labelBadgeNonCouverts->setToolTip("Exigence non referencee par aucune exigence du niveau inferieur.");

    // Badge orange cliquable : nombre de liens brises
    labelBadgeLiensBrises = new QLabel("0 Lien brise", this);
    labelBadgeLiensBrises->setStyleSheet(
        "background-color:#E67E22; color:white; border-radius:12px;"
        "padding:4px 10px; font-size:11px; font-weight:bold;");
    labelBadgeLiensBrises->setCursor(Qt::PointingHandCursor);
    labelBadgeLiensBrises->installEventFilter(this);
    labelBadgeLiensBrises->setToolTip("Reference pointant vers un identifiant inexistant.");

    // Badge violet cliquable : nombre d'orphelines
    labelBadgeOrphelines = new QLabel("0 Orpheline", this);
    labelBadgeOrphelines->setStyleSheet(
        "background-color:#7D3C98; color:white; border-radius:12px;"
        "padding:4px 10px; font-size:11px; font-weight:bold;");
    labelBadgeOrphelines->setCursor(Qt::PointingHandCursor);
    labelBadgeOrphelines->installEventFilter(this);
    labelBadgeOrphelines->setToolTip("Exigence SRS sans reference vers une exigence SSS parente.");

    layoutEntete->addWidget(labelBadgeCouverts);
    layoutEntete->addWidget(labelBadgeNonCouverts);
    layoutEntete->addWidget(labelBadgeLiensBrises);
    layoutEntete->addWidget(labelBadgeOrphelines);
    layoutEntete->addSpacing(10);

    // Bouton export avec menu deroulant (CSV, HTML, PDF)
    boutonExport = new QToolButton(this);
    boutonExport->setText("Exporter");
    boutonExport->setPopupMode(QToolButton::InstantPopup);
    boutonExport->setStyleSheet(R"(
        QToolButton {
            background-color: #27AE60; color: white;
            border-radius: 6px; padding: 6px 14px;
            font-size: 11px; font-weight: bold;
        }
        QToolButton:hover { background-color: #1E8449; }
        QToolButton::menu-indicator { image: none; }
    )");

    menuExport = new QMenu(this);
    menuExport->setStyleSheet(R"(
        QMenu {
            background-color: white; color: #333;
            border: 1px solid #AED6F1; border-radius: 6px; padding: 4px;
        }
        QMenu::item { padding: 8px 20px; border-radius: 4px; }
        QMenu::item:selected { background-color: #AED6F1; color: #1A5276; }
    )");
    menuExport->addAction("Exporter en CSV",  this, &OngletRapport::exporterCSV);
    menuExport->addAction("Exporter en HTML", this, &OngletRapport::exporterHTML);
    menuExport->addAction("Exporter en PDF",  this, &OngletRapport::exporterPDF);
    boutonExport->setMenu(menuExport);

    // Bouton filtres : affiche/cache le panneau lateral
    boutonFiltres = new QPushButton("Filtres", this);
    boutonFiltres->setStyleSheet(R"(
        QPushButton {
            background-color: #EBF5FB; color: #1A5276;
            border-radius: 6px; padding: 6px 12px;
            font-size: 11px; border: 1px solid #AED6F1;
        }
        QPushButton:hover { background-color: #AED6F1; }
    )");
    connect(boutonFiltres, &QPushButton::clicked, this, &OngletRapport::toggleFiltres);

    layoutEntete->addWidget(boutonExport);
    layoutEntete->addWidget(boutonFiltres);
    layoutPrincipal->addLayout(layoutEntete);

    // Section taux de couverture : 3 lignes label + barre de progression
    QGroupBox *groupTaux = new QGroupBox("Taux de couverture", this);
    groupTaux->setStyleSheet(R"(
        QGroupBox {
            color: #1A5276; border: 1px solid #AED6F1;
            border-radius: 8px; margin-top: 8px; padding: 10px;
            font-weight: bold; background-color: white;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
    )");
    QVBoxLayout *layoutTaux = new QVBoxLayout(groupTaux);
    layoutTaux->setSpacing(6);

    // Lambda local : cree une ligne label + barre et les ajoute au layout
    auto ajouterTaux = [&](const QString& label, QLabel*& lbl,
                           QProgressBar*& barre, const QString& couleur) {
        QHBoxLayout *row = new QHBoxLayout();
        lbl = new QLabel(label, this);
        lbl->setStyleSheet("color: #1A5276; font-size: 11px;");
        lbl->setFixedWidth(160);
        barre = creerBarre(couleur);
        row->addWidget(lbl); row->addWidget(barre);
        layoutTaux->addLayout(row);
    };

    ajouterTaux("Taux SSS \u2190 SRS : ", labelTauxSSS_SRS, barreSSS_SRS, "#4A90D9");
    ajouterTaux("Taux SRS \u2190 SDD : ", labelTauxSRS_SDD, barreSRS_SDD, "#4A90D9");
    ajouterTaux("Taux global : ",   labelTauxGlobal,  barreGlobal,  "#4A90D9");
    layoutPrincipal->addWidget(groupTaux);

    // Boutons zoom pour agrandir ou reduire les tableaux
    QHBoxLayout *layoutZoom = new QHBoxLayout();
    QString styleZoom = R"(
        QPushButton {
            background-color: #EBF5FB; color: #1A5276;
            border: 1px solid #AED6F1; border-radius: 4px;
            padding: 3px 8px; font-size: 11px;
        }
        QPushButton:hover { background-color: #AED6F1; }
    )";
    QPushButton *btnZoomPlus  = new QPushButton("Zoom +", this);
    QPushButton *btnZoomMoins = new QPushButton("Zoom -", this);
    QPushButton *btnZoomReset = new QPushButton("Reset", this);
    btnZoomPlus->setStyleSheet(styleZoom);
    btnZoomMoins->setStyleSheet(styleZoom);
    btnZoomReset->setStyleSheet(styleZoom);
    QLabel *labelZoom = new QLabel("Zoom :", this);
    labelZoom->setStyleSheet("color: #1A5276; font-size: 11px;");
    layoutZoom->addStretch();
    layoutZoom->addWidget(labelZoom);
    layoutZoom->addWidget(btnZoomMoins);
    layoutZoom->addWidget(btnZoomPlus);
    layoutZoom->addWidget(btnZoomReset);
    layoutPrincipal->addLayout(layoutZoom);

    // Onglets des matrices : SSS->SRS et SRS->SDD
    QTabWidget *tabMatrices = new QTabWidget(this);
    tabMatrices->setStyleSheet(R"(
        QTabWidget::pane { border: 1px solid #AED6F1; border-radius: 8px; background: white; }
        QTabBar::tab { background: #EBF5FB; color: #555; padding: 6px 16px; border-radius: 4px; margin-right: 4px; border: 1px solid #AED6F1; }
        QTabBar::tab:selected { background: #2E86C1; color: white; font-weight: bold; border: none; }
    )");
    tableMatriceSSS_SRS = new QTableWidget(this);
    styliserTableau(tableMatriceSSS_SRS);
    tabMatrices->addTab(tableMatriceSSS_SRS, "SSS \u2190 SRS");
    tableMatriceSRS_SDD = new QTableWidget(this);
    styliserTableau(tableMatriceSRS_SDD);
    tabMatrices->addTab(tableMatriceSRS_SDD, "SRS \u2190 SDD");
    layoutPrincipal->addWidget(tabMatrices, 1);

    // Zoom + : augmente hauteur des lignes et taille de police
    connect(btnZoomPlus, &QPushButton::clicked, this, [this]() {
        int h = tableMatriceSSS_SRS->verticalHeader()->defaultSectionSize();
        tableMatriceSSS_SRS->verticalHeader()->setDefaultSectionSize(h + 4);
        tableMatriceSRS_SDD->verticalHeader()->setDefaultSectionSize(h + 4);
        QFont f = tableMatriceSSS_SRS->font(); f.setPointSize(f.pointSize() + 1);
        tableMatriceSSS_SRS->setFont(f); tableMatriceSRS_SDD->setFont(f);
    });

    // Zoom - : reduit hauteur et police (minimum 16px et 7pt)
    connect(btnZoomMoins, &QPushButton::clicked, this, [this]() {
        int h = tableMatriceSSS_SRS->verticalHeader()->defaultSectionSize();
        if (h > 16) {
            tableMatriceSSS_SRS->verticalHeader()->setDefaultSectionSize(h - 4);
            tableMatriceSRS_SDD->verticalHeader()->setDefaultSectionSize(h - 4);
            QFont f = tableMatriceSSS_SRS->font();
            if (f.pointSize() > 7) f.setPointSize(f.pointSize() - 1);
            tableMatriceSSS_SRS->setFont(f); tableMatriceSRS_SDD->setFont(f);
        }
    });

    // Reset : remet les tableaux a leur taille d'origine (28px, 11pt)
    connect(btnZoomReset, &QPushButton::clicked, this, [this]() {
        tableMatriceSSS_SRS->verticalHeader()->setDefaultSectionSize(28);
        tableMatriceSRS_SDD->verticalHeader()->setDefaultSectionSize(28);
        QFont f = tableMatriceSSS_SRS->font(); f.setPointSize(11);
        tableMatriceSSS_SRS->setFont(f); tableMatriceSRS_SDD->setFont(f);
    });

    // Panneau liens brises : cadre orange, cache par defaut
    panneauLiensBrises = new QFrame(this);
    panneauLiensBrises->setStyleSheet(
        "QFrame { background-color:white; border:1px solid #F0B27A; border-radius:8px; }");
    panneauLiensBrises->setVisible(false);
    QVBoxLayout *layoutLB = new QVBoxLayout(panneauLiensBrises);
    layoutLB->setContentsMargins(12, 10, 12, 10); layoutLB->setSpacing(6);
    QHBoxLayout *titreLBLayout = new QHBoxLayout();
    QLabel *titreLB = new QLabel("Liens brises detectes", panneauLiensBrises);
    titreLB->setStyleSheet("color:#E67E22; font-size:12px; font-weight:bold;");
    titreLBLayout->addWidget(titreLB); titreLBLayout->addStretch();
    layoutLB->addLayout(titreLBLayout);
    listeLiensBrises = new QWidget(panneauLiensBrises);
    layoutListeLB = new QVBoxLayout(listeLiensBrises);
    layoutListeLB->setContentsMargins(0, 0, 0, 0); layoutListeLB->setSpacing(4);
    QScrollArea *scrollLB = new QScrollArea(panneauLiensBrises);
    scrollLB->setWidget(listeLiensBrises);
    scrollLB->setWidgetResizable(true);
    scrollLB->setMaximumHeight(200);
    scrollLB->setStyleSheet("QScrollArea { border: none; }");
    layoutLB->addWidget(scrollLB);
    layoutPrincipal->addWidget(panneauLiensBrises);

    // Panneau orphelines : cadre violet, cache par defaut
    panneauOrphelines = new QFrame(this);
    panneauOrphelines->setStyleSheet(
        "QFrame { background-color:white; border:1px solid #A569BD; border-radius:8px; }");
    panneauOrphelines->setVisible(false);
    QVBoxLayout *layoutOrph = new QVBoxLayout(panneauOrphelines);
    layoutOrph->setContentsMargins(12, 10, 12, 10); layoutOrph->setSpacing(6);
    QHBoxLayout *titreOrphLayout = new QHBoxLayout();
    QLabel *titreOrph = new QLabel("Exigences orphelines detectees", panneauOrphelines);
    titreOrph->setStyleSheet("color:#7D3C98; font-size:12px; font-weight:bold;");
    titreOrphLayout->addWidget(titreOrph); titreOrphLayout->addStretch();
    layoutOrph->addLayout(titreOrphLayout);
    listeOrphelines = new QWidget(panneauOrphelines);
    layoutListeOrph = new QVBoxLayout(listeOrphelines);
    layoutListeOrph->setContentsMargins(0, 0, 0, 0); layoutListeOrph->setSpacing(4);
    QScrollArea *scrollOrph = new QScrollArea(panneauOrphelines);
    scrollOrph->setWidget(listeOrphelines);
    scrollOrph->setWidgetResizable(true);
    scrollOrph->setMaximumHeight(200);
    scrollOrph->setStyleSheet("QScrollArea { border: none; }");
    layoutOrph->addWidget(scrollOrph);
    layoutPrincipal->addWidget(panneauOrphelines);

    // Panneau detail : s'affiche au clic sur une ligne de matrice
    panneauDetail = new QFrame(this);
    panneauDetail->setStyleSheet(
        "QFrame { background-color:white; border:1px solid #AED6F1; border-radius:8px; }");
    panneauDetail->setMaximumHeight(160);
    QVBoxLayout *layoutDetail = new QVBoxLayout(panneauDetail);
    layoutDetail->setContentsMargins(12, 8, 12, 8); layoutDetail->setSpacing(4);
    QLabel *titreDetail = new QLabel("EXIGENCE SELECTIONNEE", panneauDetail);
    titreDetail->setStyleSheet("color:#888; font-size:9px; font-weight:bold; letter-spacing:1px;");
    layoutDetail->addWidget(titreDetail);
    labelDetailId = new QLabel("--", panneauDetail);
    labelDetailId->setStyleSheet("color:#1A5276; font-size:16px; font-weight:bold;");
    layoutDetail->addWidget(labelDetailId);
    labelDetailDescription = new QLabel("", panneauDetail);
    labelDetailDescription->setStyleSheet("color:#555; font-size:11px;");
    labelDetailDescription->setWordWrap(true);
    layoutDetail->addWidget(labelDetailDescription);
    labelDetailEtat = new QLabel("", panneauDetail);
    layoutDetail->addWidget(labelDetailEtat);
    labelDetailFichier = new QLabel("", panneauDetail);
    labelDetailFichier->setStyleSheet("color:#888; font-size:10px; font-style:italic;");
    layoutDetail->addWidget(labelDetailFichier);
    QHBoxLayout *rowDetail = new QHBoxLayout();
    rowDetail->setSpacing(8);
    labelDetailType   = new QLabel("", panneauDetail);
    labelDetailStatut = new QLabel("", panneauDetail);
    labelDetailLiens  = new QLabel("", panneauDetail);
    labelDetailLiens->setStyleSheet("color:#1A5276; font-size:11px;");
    labelDetailLiens->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QScrollArea *scrollLiens = new QScrollArea(panneauDetail);
    scrollLiens->setWidget(labelDetailLiens);
    scrollLiens->setWidgetResizable(true);
    scrollLiens->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollLiens->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollLiens->setFixedHeight(26);
    scrollLiens->setStyleSheet(R"(
        QScrollArea { border: none; background: transparent; }
        QScrollBar:horizontal { height: 5px; background: #EBF5FB; border-radius: 3px; }
        QScrollBar::handle:horizontal { background: #AED6F1; border-radius: 3px; min-width: 20px; }
        QScrollBar::handle:horizontal:hover { background: #2E86C1; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }
    )");

    rowDetail->addWidget(labelDetailType);
    rowDetail->addWidget(labelDetailStatut);
    rowDetail->addWidget(scrollLiens, 1);
    layoutDetail->addLayout(rowDetail);
    layoutPrincipal->addWidget(panneauDetail);

    // Panneau filtres lateral : cache par defaut
    panneauFiltres = new QFrame(this);
    panneauFiltres->setStyleSheet(
        "QFrame { background-color:white; border-left:2px solid #AED6F1; }");
    panneauFiltres->setFixedWidth(220);
    panneauFiltres->setVisible(false);
    QVBoxLayout *layoutFiltres = new QVBoxLayout(panneauFiltres);
    layoutFiltres->setContentsMargins(0, 0, 0, 0);
    gestionnaireFiltres = new GestionnaireFiltresGUI(panneauFiltres);
    layoutFiltres->addWidget(gestionnaireFiltres);
    connect(gestionnaireFiltres, &GestionnaireFiltresGUI::filtreModifie,
            this, &OngletRapport::appliquerFiltre);

    layoutGlobal->addWidget(contenu);
    layoutGlobal->addWidget(panneauFiltres);
    setLayout(layoutGlobal);
}


/**
 * \brief Affiche ou cache le panneau de filtres lateral.
 *
 * Met aussi a jour le texte du bouton selon l'etat (> ou <).
 */
void OngletRapport::toggleFiltres()
{
    bool visible = panneauFiltres->isVisible();
    panneauFiltres->setVisible(!visible);
    boutonFiltres->setText(visible ? "Filtres >" : "Filtres <");
}

/**
 * \brief Intercepte les clics sur les badges "Lien brise" et "Orpheline".
 *
 * Un clic sur le badge orange affiche ou cache le panneau liens brises.
 * Un clic sur le badge violet affiche ou cache le panneau orphelines.
 *
 * \param obj   Le widget sur lequel l'evenement s'est produit.
 * \param event L'evenement intercepte.
 * \return true si l'evenement a ete traite, false sinon.
 */
bool OngletRapport::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (obj == labelBadgeLiensBrises) {
            panneauLiensBrises->setVisible(!panneauLiensBrises->isVisible());
            return true;
        }
        if (obj == labelBadgeOrphelines) {
            panneauOrphelines->setVisible(!panneauOrphelines->isVisible());
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}


/**
 * \brief Surcharge simplifiee sans donnees SRS/SDD.
 *
 * Appelle afficherRapportComplet avec des vecteurs vides.
 *
 * \param rapport Le rapport produit par MoteurTracabilite.
 */
void OngletRapport::afficherRapport(const RapportTracabilite& rapport)
{
    afficherRapportComplet(rapport, {}, {}, {});
}

/**
 * \brief Remplit tous les widgets de l'onglet avec les donnees du rapport.
 *
 * Met a jour les taux, les matrices SSS x SRS et SRS x SDD (3 colonnes chacune),
 * les badges de resume, les panneaux liens brises et orphelines,
 * et connecte le clic sur les lignes au panneau detail.
 *
 * \param rapport    Le rapport produit par MoteurTracabilite.
 * \param srs        Les exigences SRS analysees.
 * \param sdd        Les elements SDD analyses.
 * \param orphelines Les IDs des exigences SRS sans parent SSS.
 */
void OngletRapport::afficherRapportComplet(const RapportTracabilite& rapport,
                                           const std::vector<ExigenceSRS>& srs,
                                           const std::vector<ExigenceSDD>& sdd,
                                           const std::vector<std::string>& orphelines)
{
    // Sauvegarde des donnees pour les exports et le filtrage ulterieur
    rapportCourant     = rapport;
    srsAnalyses        = srs;
    sddAnalyses        = sdd;
    orphelinesAnalyses = orphelines;

    // Mise a jour des taux (conversion 0.0-1.0 en pourcentage entier)
    int pSSS = qRound(rapport.tauxSSS_SRS * 100);
    int pSRS = qRound(rapport.tauxSRS_SDD * 100);
    int pGlb = qRound(rapport.tauxGlobal  * 100);
    labelTauxSSS_SRS->setText(QString("Taux SSS \u2190 SRS : %1 %").arg(pSSS));
    labelTauxSRS_SDD->setText(QString("Taux SRS \u2190 SDD : %1 %").arg(pSRS));
    labelTauxGlobal ->setText(QString("Taux global : %1 %")  .arg(pGlb));
    barreSSS_SRS->setValue(pSSS); barreSRS_SDD->setValue(pSRS); barreGlobal->setValue(pGlb);

    // Remplissage de la matrice SSS x SRS (3 colonnes : Exigence | Statut | Couverte par)
    int nbSSS = (int)rapport.exigencesSSS.size();
    tableMatriceSSS_SRS->setRowCount(nbSSS);
    tableMatriceSSS_SRS->setColumnCount(3);
    tableMatriceSSS_SRS->setHorizontalHeaderItem(0, new QTableWidgetItem("Exigence SSS"));
    tableMatriceSSS_SRS->setHorizontalHeaderItem(1, new QTableWidgetItem("Statut"));
    tableMatriceSSS_SRS->setHorizontalHeaderItem(2, new QTableWidgetItem("Couverte par (SRS)"));
    tableMatriceSSS_SRS->verticalHeader()->setVisible(false);
    tableMatriceSSS_SRS->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tableMatriceSSS_SRS->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tableMatriceSSS_SRS->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    QStringList tousLesIds;
    int nbCouverts = 0, nbNonCouverts = 0;

    for (int i = 0; i < nbSSS; i++) {
        const ExigenceSSS& exig = rapport.exigencesSSS[i];
        Statut statut = exig.getStatut();
        if (statut == Couverte)    nbCouverts++;
        if (statut == NonCouverte) nbNonCouverts++;
        tousLesIds << QString::fromStdString(exig.getId());

        // Colonne 0 : identifiant SSS
        QTableWidgetItem *cellId = new QTableWidgetItem(QString::fromStdString(exig.getId()));
        cellId->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        cellId->setFlags(cellId->flags() & ~Qt::ItemIsEditable);
        tableMatriceSSS_SRS->setItem(i, 0, cellId);

        // Verification si cette exigence est impliquee dans un lien brise
        // Comparaison exacte sur l'ID source du lien brisÃ©
        bool lienBrise = false;
        for (const std::string& lb : rapport.liensBrises) {
            size_t debut = lb.find(": ");
            size_t fin   = lb.find(" [");
            if (debut != std::string::npos && fin != std::string::npos) {
                std::string idSource = lb.substr(debut + 2, fin - debut - 2);
                if (idSource == exig.getId()) { lienBrise = true; break; }
            }
        }

        // Colonne 1 : statut colore (orange si lien brise, vert si couvert, rouge sinon)
        QTableWidgetItem *cellStatut;
        if (lienBrise) {
            cellStatut = new QTableWidgetItem("Lien brise");
            cellStatut->setBackground(QColor("#E67E22")); cellStatut->setForeground(Qt::white);
        } else if (statut == Couverte) {
            cellStatut = new QTableWidgetItem("Couvert");
            cellStatut->setBackground(QColor("#27AE60")); cellStatut->setForeground(Qt::white);
        } else {
            cellStatut = new QTableWidgetItem("Non couvert");
            cellStatut->setBackground(QColor("#E74C3C")); cellStatut->setForeground(Qt::white);
        }
        cellStatut->setTextAlignment(Qt::AlignCenter);
        cellStatut->setFlags(cellStatut->flags() & ~Qt::ItemIsEditable);
        tableMatriceSSS_SRS->setItem(i, 1, cellStatut);

        // Colonne 2 : IDs des SRS qui couvrent cette exigence SSS
        QStringList srsList;
        for (const ExigenceSRS& s : srs) {
            const auto& t = s.getTracabilite();
            if (std::find(t.begin(), t.end(), exig.getId()) != t.end())
                srsList << QString::fromStdString(s.getId());
        }
        QTableWidgetItem *cellSRS = new QTableWidgetItem(srsList.isEmpty() ? "--" : srsList.join(", "));
        cellSRS->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        cellSRS->setFlags(cellSRS->flags() & ~Qt::ItemIsEditable);
        if (srsList.isEmpty()) cellSRS->setForeground(QColor("#AAAAAA"));
        tableMatriceSSS_SRS->setItem(i, 2, cellSRS);
    }

    // Remplissage de la matrice SRS x SDD (3 colonnes : Exigence | Statut | Couverte par)
    int nbSRS = (int)srs.size();
    tableMatriceSRS_SDD->setRowCount(nbSRS > 0 ? nbSRS : 1);
    tableMatriceSRS_SDD->setColumnCount(3);
    tableMatriceSRS_SDD->setHorizontalHeaderItem(0, new QTableWidgetItem("Exigence SRS"));
    tableMatriceSRS_SDD->setHorizontalHeaderItem(1, new QTableWidgetItem("Statut"));
    tableMatriceSRS_SDD->setHorizontalHeaderItem(2, new QTableWidgetItem("Couverte par (SDD)"));
    tableMatriceSRS_SDD->verticalHeader()->setVisible(false);
    tableMatriceSRS_SDD->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tableMatriceSRS_SDD->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tableMatriceSRS_SDD->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    int nbCouvertsSRS = 0, nbNonCouvertsSRS = 0;

    if (nbSRS > 0) {
        for (int i = 0; i < nbSRS; i++) {
            const ExigenceSRS& exig = srs[i];
            tousLesIds << QString::fromStdString(exig.getId());

            // Colonne 0 : identifiant SRS
            QTableWidgetItem *cellId = new QTableWidgetItem(QString::fromStdString(exig.getId()));
            cellId->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
            cellId->setFlags(cellId->flags() & ~Qt::ItemIsEditable);
            tableMatriceSRS_SDD->setItem(i, 0, cellId);

            // Collecte des SDD qui referencent cette SRS
            QStringList sddList;
            for (const ExigenceSDD& d : sdd) {
                const auto& ref = d.getReferencedSRS();
                if (std::find(ref.begin(), ref.end(), exig.getId()) != ref.end())
                    sddList << QString::fromStdString(d.getId());
            }
            bool estCouvert  = !sddList.isEmpty();
            bool estOrphelin = exig.getTracabilite().empty();

            // Comparaison exacte sur l'ID source du lien brisÃ©
            bool estLienBris = false;
            for (const std::string& lb : rapport.liensBrises) {
                size_t debut = lb.find(": ");
                size_t fin   = lb.find(" [");
                if (debut != std::string::npos && fin != std::string::npos) {
                    std::string idSource = lb.substr(debut + 2, fin - debut - 2);
                    if (idSource == exig.getId()) { estLienBris = true; break; }
                }
            }

            if (estCouvert)                  nbCouvertsSRS++;
            if (!estCouvert && !estLienBris) nbNonCouvertsSRS++;

            // Colonne 1 : priorite lien brise > orpheline > couvert > non couvert
            QTableWidgetItem *cellStatut;
            if (estLienBris) {
                cellStatut = new QTableWidgetItem("Lien brise");
                cellStatut->setBackground(QColor("#E67E22")); cellStatut->setForeground(Qt::white);
            } else if (estOrphelin) {
                cellStatut = new QTableWidgetItem("Orpheline");
                cellStatut->setBackground(QColor("#7D3C98")); cellStatut->setForeground(Qt::white);
            } else if (estCouvert) {
                cellStatut = new QTableWidgetItem("Couvert");
                cellStatut->setBackground(QColor("#27AE60")); cellStatut->setForeground(Qt::white);
            } else {
                cellStatut = new QTableWidgetItem("Non couvert");
                cellStatut->setBackground(QColor("#E74C3C")); cellStatut->setForeground(Qt::white);
            }
            cellStatut->setTextAlignment(Qt::AlignCenter);
            cellStatut->setFlags(cellStatut->flags() & ~Qt::ItemIsEditable);
            tableMatriceSRS_SDD->setItem(i, 1, cellStatut);

            // Colonne 2 : IDs des SDD qui couvrent cette SRS
            QTableWidgetItem *cellSDD = new QTableWidgetItem(sddList.isEmpty() ? "--" : sddList.join(", "));
            cellSDD->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
            cellSDD->setFlags(cellSDD->flags() & ~Qt::ItemIsEditable);
            if (sddList.isEmpty()) cellSDD->setForeground(QColor("#AAAAAA"));
            tableMatriceSRS_SDD->setItem(i, 2, cellSDD);
        }
    } else {
        QTableWidgetItem *msg = new QTableWidgetItem("Aucun fichier SDD importe");
        msg->setForeground(QColor("#AAAAAA"));
        tableMatriceSRS_SDD->setItem(0, 0, msg);
    }

    gestionnaireFiltres->setListeIds(tousLesIds);

    // Mise a jour des badges de resume
    labelBadgeCouverts->setText(QString::number(nbCouverts + nbCouvertsSRS) + " Couvert");
    labelBadgeNonCouverts->setText(QString::number(nbNonCouverts + nbNonCouvertsSRS) + " Non couvert");
    labelBadgeLiensBrises->setText(QString::number(rapport.liensBrises.size()) + " Lien brise");
    labelBadgeOrphelines->setText(QString::number(orphelines.size()) + " Orpheline");

    // Remplissage du panneau liens brises : une ligne par lien
    QLayoutItem* item;
    while ((item = layoutListeLB->takeAt(0)) != nullptr) { delete item->widget(); delete item; }
    panneauLiensBrises->setVisible(!rapport.liensBrises.empty());
    for (const std::string& lb : rapport.liensBrises) {
        QFrame *ligne = new QFrame(listeLiensBrises);
        ligne->setStyleSheet("QFrame { background-color:#FEF9F0; border:1px solid #F0B27A; border-radius:6px; }");
        QHBoxLayout *row = new QHBoxLayout(ligne);
        row->setContentsMargins(10, 6, 10, 6); row->setSpacing(10);
        QLabel *texte = new QLabel(QString::fromStdString(lb), ligne);
        texte->setStyleSheet("color:#A04000; font-size:11px;"); texte->setWordWrap(true);
        row->addWidget(texte, 1);
        layoutListeLB->addWidget(ligne);
    }

    // Remplissage du panneau orphelines : une ligne par orpheline
    QLayoutItem* itemOrph;
    while ((itemOrph = layoutListeOrph->takeAt(0)) != nullptr) { delete itemOrph->widget(); delete itemOrph; }
    panneauOrphelines->setVisible(!orphelines.empty());
    for (const std::string& orph : orphelines) {
        QFrame *ligne = new QFrame(listeOrphelines);
        ligne->setStyleSheet("QFrame { background-color:#F5EEF8; border:1px solid #A569BD; border-radius:6px; }");
        QHBoxLayout *row = new QHBoxLayout(ligne);
        row->setContentsMargins(10, 6, 10, 6); row->setSpacing(10);
        QLabel *texte = new QLabel(QString::fromStdString(orph), ligne);
        texte->setStyleSheet("color:#7D3C98; font-size:11px;"); texte->setWordWrap(true);
        row->addWidget(texte, 1);
        layoutListeOrph->addWidget(ligne);
    }

    // Clic sur la matrice SSS->SRS : affiche les details dans le panneau du bas
    connect(tableMatriceSSS_SRS, &QTableWidget::cellClicked, this, [this](int row, int) {
        if (row >= (int)rapportCourant.exigencesSSS.size()) return;
        const ExigenceSSS& exig = rapportCourant.exigencesSSS[row];
        labelDetailId->setText(QString::fromStdString(exig.getId()));
        labelDetailDescription->setText(QString::fromStdString(exig.getContenu()));
        QString etatSSS = QString::fromStdString(exig.getEtat());
        QString methSSS = QString::fromStdString(exig.getMethodeVerification());
        QString infoSSS = etatSSS;
        if (!methSSS.isEmpty()) infoSSS += "  |  " + methSSS;
        labelDetailEtat->setText(infoSSS);
        labelDetailEtat->setStyleSheet("color:#27AE60; font-size:11px; font-weight:bold;");
        labelDetailFichier->setText(QString::fromStdString(exig.getFichierSource()));
        labelDetailType->setText(" SSS ");
        labelDetailType->setStyleSheet("background-color:#D6EAF8; color:#1A5276; border-radius:6px; padding:2px 8px; font-size:11px; font-weight:bold;");
        QString statutStr; QString styleStatut;
        switch (exig.getStatut()) {
        case Couverte:    statutStr=" Couvert ";    styleStatut="background-color:#27AE60;color:white;border-radius:6px;padding:2px 8px;font-size:11px;font-weight:bold;"; break;
        case NonCouverte: statutStr=" Non couvert ";styleStatut="background-color:#E74C3C;color:white;border-radius:6px;padding:2px 8px;font-size:11px;font-weight:bold;"; break;
        default:          statutStr=" Non analyse ";styleStatut="background-color:#AAAAAA;color:white;border-radius:6px;padding:2px 8px;font-size:11px;font-weight:bold;"; break;
        }
        labelDetailStatut->setText(statutStr); labelDetailStatut->setStyleSheet(styleStatut);
        QStringList liens;
        for (const ExigenceSRS& s : srsAnalyses) {
            const auto& t = s.getTracabilite();
            if (std::find(t.begin(), t.end(), exig.getId()) != t.end())
                liens << QString::fromStdString(s.getId());
        }
        labelDetailLiens->setText(liens.isEmpty() ? "Aucun lien SRS"
                                                  : QString::number(liens.size()) + " lien(s) : " + liens.join(", "));
    });

    // Clic sur la matrice SRS->SDD : affiche les details dans le panneau du bas
    connect(tableMatriceSRS_SDD, &QTableWidget::cellClicked, this, [this](int row, int) {
        if (row >= (int)srsAnalyses.size()) return;
        const ExigenceSRS& exig = srsAnalyses[row];
        labelDetailId->setText(QString::fromStdString(exig.getId()));
        labelDetailDescription->setText(QString::fromStdString(exig.getContenu()));
        QString etatSRS = QString::fromStdString(exig.getEtat());
        QString methSRS = QString::fromStdString(exig.getMethodeVerification());
        QStringList ciblesSRS;
        for (const auto& c : exig.getCibles()) ciblesSRS << QString::fromStdString(c);
        QString infoSRS = etatSRS;
        if (!methSRS.isEmpty()) infoSRS += "  |  " + methSRS;
        if (!ciblesSRS.isEmpty()) infoSRS += "  |  Cible : " + ciblesSRS.join(", ");
        labelDetailEtat->setText(infoSRS);
        labelDetailEtat->setStyleSheet("color:#27AE60; font-size:11px; font-weight:bold;");
        labelDetailFichier->setText(QString::fromStdString(exig.getFichierSource()));
        labelDetailType->setText(" SRS ");
        labelDetailType->setStyleSheet("background-color:#FAE5D3; color:#E67E22; border-radius:6px; padding:2px 8px; font-size:11px; font-weight:bold;");
        QStringList liens;
        for (const ExigenceSDD& d : sddAnalyses) {
            const auto& ref = d.getReferencedSRS();
            if (std::find(ref.begin(), ref.end(), exig.getId()) != ref.end())
                liens << QString::fromStdString(d.getId());
        }
        bool estCouvert  = !liens.isEmpty();
        bool estOrphelin = exig.getTracabilite().empty();
        bool estLienBris = false;
        for (const std::string& lb : rapportCourant.liensBrises)
            if (lb.find(exig.getId()) != std::string::npos) { estLienBris = true; break; }
        QString statutStr; QString styleStatut;
        if (estLienBris)       { statutStr=" Lien brise ";  styleStatut="background-color:#E67E22;color:white;border-radius:6px;padding:2px 8px;font-size:11px;font-weight:bold;"; }
        else if (estOrphelin)  { statutStr=" Orpheline ";   styleStatut="background-color:#7D3C98;color:white;border-radius:6px;padding:2px 8px;font-size:11px;font-weight:bold;"; }
        else if (estCouvert)   { statutStr=" Couvert ";     styleStatut="background-color:#27AE60;color:white;border-radius:6px;padding:2px 8px;font-size:11px;font-weight:bold;"; }
        else                   { statutStr=" Non couvert "; styleStatut="background-color:#E74C3C;color:white;border-radius:6px;padding:2px 8px;font-size:11px;font-weight:bold;"; }
        labelDetailStatut->setText(statutStr); labelDetailStatut->setStyleSheet(styleStatut);
        labelDetailLiens->setText(liens.isEmpty() ? "Aucun lien SDD"
                                                  : QString::number(liens.size()) + " lien(s) : " + liens.join(", "));
    });
}

/**
 * \brief Applique les criteres de filtre sur les deux matrices.
 *
 * Cache les lignes qui ne correspondent pas aux criteres actifs.
 * Criteres possibles : identifiant (texte libre), statut (Couverte/NonCouverte),
 * etat de developpement, et orpheline.
 *
 * \param filtre L'objet FiltreGUI contenant tous les criteres actifs.
 */
void OngletRapport::appliquerFiltre(const FiltreGUI& filtre)
{
    // â”€â”€ Matrice SSS->SRS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    for (int i = 0; i < tableMatriceSSS_SRS->rowCount(); i++) {
        bool afficher = true;

        if (!filtre.filtreIdentifiant.isEmpty()) {
            QString recherche = filtre.filtreIdentifiant;
            bool trouve = false;
            // ID
            QTableWidgetItem* it = tableMatriceSSS_SRS->item(i, 0);
            if (it && it->text().contains(recherche, Qt::CaseInsensitive)) trouve = true;
            // MÃ©thode de vÃ©rification + Ã©tat
            if (!trouve && i < (int)rapportCourant.exigencesSSS.size()) {
                QString methode = QString::fromStdString(rapportCourant.exigencesSSS[i].getMethodeVerification());
                QString etat    = QString::fromStdString(rapportCourant.exigencesSSS[i].getEtat());
                if (methode.contains(recherche, Qt::CaseInsensitive)) trouve = true;
                if (etat.contains(recherche, Qt::CaseInsensitive))    trouve = true;
            }
            afficher = trouve;
        }

        if (afficher && i < (int)rapportCourant.exigencesSSS.size()) {
            const ExigenceSSS& exig = rapportCourant.exigencesSSS[i];

            bool lienBrise = false;
            for (const std::string& lb : rapportCourant.liensBrises)
                if (lb.find(exig.getId()) != std::string::npos) { lienBrise = true; break; }

            bool couverte    = (exig.getStatut() == Couverte)    && !lienBrise;
            bool nonCouverte = (exig.getStatut() == NonCouverte) && !lienBrise;
            bool orpheline   = false; // SSS ne peut pas etre orpheline

            if (filtre.filtreLienBrise   && !lienBrise)   afficher = false;
            if (filtre.filtreOrpheline   && !orpheline)   afficher = false;
            if (filtre.filtreStatut == Couverte    && !couverte)    afficher = false;
            if (filtre.filtreStatut == NonCouverte && !nonCouverte) afficher = false;

            if (afficher && filtre.filtreDevEtat && !filtre.filtreEnCours && i < (int)rapportCourant.exigencesSSS.size()) {
                if (filtre.filtreDevValeur != rapportCourant.exigencesSSS[i].getDeveloppe()) afficher = false;
            }
            if (afficher && filtre.filtreEnCours && i < (int)rapportCourant.exigencesSSS.size()) {
                if (rapportCourant.exigencesSSS[i].getEtat() != "En cours") afficher = false;
            }
        }

        tableMatriceSSS_SRS->setRowHidden(i, !afficher);
    }

    // â”€â”€ Matrice SRS->SDD â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    for (int i = 0; i < tableMatriceSRS_SDD->rowCount(); i++) {
        bool afficher = true;

        if (!filtre.filtreIdentifiant.isEmpty()) {
            QString recherche = filtre.filtreIdentifiant;
            bool trouve = false;
            // ID
            QTableWidgetItem* it = tableMatriceSRS_SDD->item(i, 0);
            if (it && it->text().contains(recherche, Qt::CaseInsensitive)) trouve = true;
            // Cibles
            if (!trouve && i < (int)srsAnalyses.size()) {
                for (const auto& cible : srsAnalyses[i].getCibles()) {
                    if (QString::fromStdString(cible).contains(recherche, Qt::CaseInsensitive))
                    { trouve = true; break; }
                }
            }
            // MÃ©thode de vÃ©rification + Ã©tat
            if (!trouve && i < (int)srsAnalyses.size()) {
                QString methode = QString::fromStdString(srsAnalyses[i].getMethodeVerification());
                QString etat    = QString::fromStdString(srsAnalyses[i].getEtat());
                if (methode.contains(recherche, Qt::CaseInsensitive)) trouve = true;
                if (etat.contains(recherche, Qt::CaseInsensitive))    trouve = true;
            }
            afficher = trouve;
        }

        if (afficher && i < (int)srsAnalyses.size()) {
            const ExigenceSRS& exig = srsAnalyses[i];

            bool lienBrise = false;
            for (const std::string& lb : rapportCourant.liensBrises)
                if (lb.find(exig.getId()) != std::string::npos) { lienBrise = true; break; }

            bool orpheline = exig.getTracabilite().empty() && !lienBrise;

            bool couverte = false;
            for (const ExigenceSDD& d : sddAnalyses) {
                const auto& ref = d.getReferencedSRS();
                if (std::find(ref.begin(), ref.end(), exig.getId()) != ref.end())
                { couverte = true; break; }
            }
            couverte = couverte && !lienBrise;
            bool nonCouverte = !couverte && !lienBrise && !orpheline;

            if (filtre.filtreLienBrise   && !lienBrise)   afficher = false;
            if (filtre.filtreOrpheline   && !orpheline)   afficher = false;
            if (filtre.filtreStatut == Couverte    && !couverte)    afficher = false;
            if (filtre.filtreStatut == NonCouverte && !nonCouverte) afficher = false;

            if (afficher && filtre.filtreDevEtat && !filtre.filtreEnCours && i < (int)srsAnalyses.size()) {
                if (filtre.filtreDevValeur != srsAnalyses[i].getDeveloppe()) afficher = false;
            }
            if (afficher && filtre.filtreEnCours && i < (int)srsAnalyses.size()) {
                if (srsAnalyses[i].getEtat() != "En cours") afficher = false;
            }
        }

        tableMatriceSRS_SDD->setRowHidden(i, !afficher);
    }
}


// =============================================================================
// Helpers prives pour les exports
// =============================================================================

/**
 * \brief Retourne la couleur associee a un taux de couverture.
 *
 * Vert si >= 80 %, orange si >= 50 %, rouge sinon.
 *
 * \param p Taux en pourcentage entier (0-100).
 * \return Chaine hexadecimale de la couleur.
 */
static QString couleurTaux(int p)
{
    return p >= 80 ? "#27AE60" : p >= 50 ? "#E67E22" : "#E74C3C";
}

/**
 * \brief Genere un badge HTML inline colore pour les exports PDF/HTML.
 *
 * \param txt Texte affiche dans le badge.
 * \param bg  Couleur de fond hexadecimale.
 * \return Chaine HTML du badge.
 *
 * \author Amel Kheloul
 */
static QString badgeSpan(const QString& txt, const QString& bg)
{
    return "<span style='background:" + bg + "; color:white; border-radius:4px;"
                                             "padding:2px 8px; font-size:10px; font-weight:bold'>" + txt + "</span>";
}


/**
 * \brief Exporte le rapport complet au format CSV.
 *
 * Le fichier contient les taux, la matrice SSS x SRS, la matrice SRS x SDD,
 * la liste des liens brises et la liste des orphelines si presentes.
 * Ouvre une boite de dialogue "Enregistrer sous" avant d'ecrire.
 *
 * Apport Amel Kheloul : ajout de la section orphelines en fin de fichier.
 */
void OngletRapport::exporterCSV()
{
    QString chemin = QFileDialog::getSaveFileName(
        this, "Exporter en CSV", "rapport_tracabilite.csv", "Fichiers CSV (*.csv)");
    if (chemin.isEmpty()) return;
    QFile fichier(chemin);
    if (!fichier.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Erreur", "Impossible d'ecrire le fichier."); return;
    }
    QTextStream out(&fichier);
    out.setEncoding(QStringConverter::Utf8);
    out << "=== RAPPORT DE TRACABILITE TRACELINK ===\n";
    out << "Date;" << QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm") << "\n";
    out << "Taux SSS->SRS;" << QString::number(rapportCourant.tauxSSS_SRS * 100, 'f', 1) << "%\n";
    out << "Taux SRS->SDD;" << QString::number(rapportCourant.tauxSRS_SDD * 100, 'f', 1) << "%\n";
    out << "Taux global;"   << QString::number(rapportCourant.tauxGlobal * 100, 'f', 1)   << "%\n\n";
    out << "=== MATRICE SSS x SRS ===\n";
    out << "Identifiant SSS;Statut;Couverte par (SRS);Lien brise\n";
    for (const ExigenceSSS& exig : rapportCourant.exigencesSSS) {
        QString statut;
        switch (exig.getStatut()) {
        case Couverte:    statut = "Couverte";     break;
        case NonCouverte: statut = "Non couverte"; break;
        default:          statut = "Non analyse";  break;
        }
        QStringList srsList;
        for (const ExigenceSRS& s : srsAnalyses) {
            const auto& t = s.getTracabilite();
            if (std::find(t.begin(), t.end(), exig.getId()) != t.end())
                srsList << QString::fromStdString(s.getId());
        }
        bool lienBrise = false;
        for (const std::string& lb : rapportCourant.liensBrises)
            if (lb.find(exig.getId()) != std::string::npos) { lienBrise = true; break; }
        out << QString::fromStdString(exig.getId()) << ";" << statut << ";"
            << (srsList.isEmpty() ? "-" : srsList.join(" | ")) << ";"
            << (lienBrise ? "OUI" : "NON") << "\n";
    }
    out << "\n=== MATRICE SRS x SDD ===\n";
    out << "Identifiant SRS;Statut;Couverte par (SDD);Orpheline\n";
    for (const ExigenceSRS& exig : srsAnalyses) {
        QStringList sddList;
        for (const ExigenceSDD& d : sddAnalyses) {
            const auto& ref = d.getReferencedSRS();
            if (std::find(ref.begin(), ref.end(), exig.getId()) != ref.end())
                sddList << QString::fromStdString(d.getId());
        }
        bool couvert   = !sddList.isEmpty();
        bool orpheline = exig.getTracabilite().empty();
        out << QString::fromStdString(exig.getId()) << ";"
            << (couvert ? "Couverte" : "Non couverte") << ";"
            << (sddList.isEmpty() ? "-" : sddList.join(" | ")) << ";"
            << (orpheline ? "OUI" : "NON") << "\n";
    }
    if (!rapportCourant.liensBrises.empty()) {
        out << "\n=== LIENS BRISES ===\n";
        for (const std::string& lb : rapportCourant.liensBrises)
            out << QString::fromStdString(lb) << "\n";
    }
    // Apport Amel Kheloul : section orphelines
    if (!orphelinesAnalyses.empty()) {
        out << "\n=== EXIGENCES ORPHELINES ===\n";
        for (const std::string& o : orphelinesAnalyses)
            out << QString::fromStdString(o) << "\n";
    }
    fichier.close();
    QMessageBox msg(this);
    msg.setWindowTitle("Export CSV");
    msg.setText("Fichier CSV export\u00e9 avec succ\u00e8s !");
    msg.setStyleSheet("QMessageBox { background-color: white; } QMessageBox QLabel { color: black; }");
    msg.exec();
}


/**
 * \brief Exporte le rapport au format HTML interactif.
 *
 * La page generee contient un tableau a 3 colonnes :
 *   Exigence    : identifiant de l'exigence
 *   Statut      : Couvert, Non couvert, Lien brise, Orpheline avec code couleur
 *   Couverte par : IDs des exigences du niveau suivant
 *
 * Fonctionnalites JavaScript incluses dans la page :
 *   Recherche en temps reel sur tout le texte de la ligne
 *   Select dropdown pour filtrer par statut
 *   Bouton Reinitialiser
 *   Legende coloree
 *   Onglets SSS->SRS et SRS->SDD
 *   Compteur d'exigences affichees
 *   Barres de progression animees pour les taux
 *
 * \author Lamia Arrahmane
 * Apports Amel Kheloul : select dropdown statut, bouton Reset, legende,
 *                        recherche sur tout le texte de la ligne, CSS compact.
 */
void OngletRapport::exporterHTML()
{
    QString chemin = QFileDialog::getSaveFileName(
        this, "Exporter en HTML", "rapport_tracabilite.html", "Fichiers HTML (*.html)");
    if (chemin.isEmpty()) return;

    QFile fichier(chemin);
    if (!fichier.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Erreur", "Impossible d'ecrire le fichier."); return;
    }
    QTextStream out(&fichier);
    out.setEncoding(QStringConverter::Utf8);

    int pSSS = qRound(rapportCourant.tauxSSS_SRS * 100);
    int pSRS = qRound(rapportCourant.tauxSRS_SDD * 100);
    int pGlb = qRound(rapportCourant.tauxGlobal  * 100);

    // Construction des lignes HTML pour la matrice SSS->SRS
    QString lignesSSS = "";
    for (const ExigenceSSS& exig : rapportCourant.exigencesSSS) {
        bool lienBrise = false;
        for (const std::string& lb : rapportCourant.liensBrises)
            if (lb.find(exig.getId()) != std::string::npos) { lienBrise = true; break; }

        QString classe = lienBrise                        ? "lienbrise"   :
                             exig.getStatut() == Couverte     ? "couvert"     : "noncouvert";
        QString statut = lienBrise                        ? "Lien brise"  :
                             exig.getStatut() == Couverte     ? "Couvert"     : "Non couvert";

        QStringList srsList;
        for (const ExigenceSRS& s : srsAnalyses) {
            const auto& t = s.getTracabilite();
            if (std::find(t.begin(), t.end(), exig.getId()) != t.end())
                srsList << QString::fromStdString(s.getId());
        }
        QString couvertePar = srsList.isEmpty()
                                  ? "<span class=\"vide\">--</span>"
                                  : srsList.join(", ");

        lignesSSS += QString("<tr data-statut=\"%1\">"
                             "<td class=\"id-col\">%2</td>"
                             "<td><span class=\"badge %1\">%3</span></td>"
                             "<td>%4</td>"
                             "</tr>\n")
                         .arg(classe)
                         .arg(QString::fromStdString(exig.getId()))
                         .arg(statut)
                         .arg(couvertePar);
    }

    // Construction des lignes HTML pour la matrice SRS->SDD
    QString lignesSRS = "";
    for (const ExigenceSRS& exig : srsAnalyses) {
        QStringList sddList;
        for (const ExigenceSDD& d : sddAnalyses) {
            const auto& ref = d.getReferencedSRS();
            if (std::find(ref.begin(), ref.end(), exig.getId()) != ref.end())
                sddList << QString::fromStdString(d.getId());
        }
        bool estCouvert  = !sddList.isEmpty();
        bool estOrphelin = exig.getTracabilite().empty();
        bool estLienBris = false;
        for (const std::string& lb : rapportCourant.liensBrises)
            if (lb.find(exig.getId()) != std::string::npos) { estLienBris = true; break; }

        QString classe = estLienBris  ? "lienbrise"  :
                             estOrphelin  ? "orpheline"  :
                             estCouvert   ? "couvert"    : "noncouvert";
        QString statut = estLienBris  ? "Lien brise" :
                             estOrphelin  ? "Orpheline"  :
                             estCouvert   ? "Couvert"    : "Non couvert";

        QString couvertePar = sddList.isEmpty()
                                  ? "<span class=\"vide\">--</span>"
                                  : sddList.join(", ");

        lignesSRS += QString("<tr data-statut=\"%1\">"
                             "<td class=\"id-col\">%2</td>"
                             "<td><span class=\"badge %1\">%3</span></td>"
                             "<td>%4</td>"
                             "</tr>\n")
                         .arg(classe)
                         .arg(QString::fromStdString(exig.getId()))
                         .arg(statut)
                         .arg(couvertePar);
    }

    // Valeurs textuelles des badges
    QString nbCouv    = labelBadgeCouverts->text().split(" ")[0];
    QString nbNonCouv = labelBadgeNonCouverts->text().split(" ")[0];
    QString nbLB      = labelBadgeLiensBrises->text().split(" ")[0];
    QString nbOrph    = labelBadgeOrphelines->text().split(" ")[0];

    // Generation du HTML complet avec CSS inline et JavaScript
    out << "<!DOCTYPE html>\n"
        << "<html lang=\"fr\">\n"
        << "<head>\n"
        << "<meta charset=\"UTF-8\">\n"
        << "<title>Rapport TraceLink</title>\n"
        << "<style>\n"
        << "* { box-sizing:border-box; margin:0; padding:0; }\n"
        << "body { font-family:Arial,sans-serif; background:#EBF5FB; color:#333; padding:24px; }\n"
        << "h1 { color:#1A5276; border-bottom:2px solid #AED6F1; padding-bottom:10px; margin-bottom:16px; font-size:20px; }\n"
        << "h2 { color:#1A5276; margin:24px 0 10px; font-size:15px; }\n"
        << ".header { background:linear-gradient(135deg,#1A5276,#2E86C1); color:white; padding:24px 32px; margin:-24px -24px 24px; }\n"
        << ".header h1 { color:white; border:none; font-size:26px; font-weight:900; letter-spacing:3px; margin:0; }\n"
        << ".header p { font-size:11px; opacity:0.75; margin-top:4px; }\n"
        << ".badges { display:flex; gap:8px; margin-bottom:16px; flex-wrap:wrap; }\n"
        << ".badge-sum { padding:4px 14px; border-radius:12px; color:white; font-size:12px; font-weight:bold; }\n"
        << ".taux-row { display:flex; gap:14px; flex-wrap:wrap; margin-bottom:20px; }\n"
        << ".taux-card { flex:1; background:white; border-radius:8px; padding:12px 16px; border:1px solid #AED6F1; min-width:160px; }\n"
        << ".taux-label { font-size:11px; color:#555; margin-bottom:4px; }\n"
        << ".taux-val { font-size:22px; font-weight:bold; }\n"
        << ".prog { background:#D6EAF8; border-radius:6px; height:8px; margin-top:6px; }\n"
        << ".prog-bar { height:8px; border-radius:6px; width:0%; transition:width 1s ease; }\n"
        << ".toolbar { display:flex; gap:10px; align-items:center; flex-wrap:wrap; background:white; padding:10px 14px; border-radius:8px; border:1px solid #AED6F1; margin-bottom:8px; }\n"
        << ".toolbar label { font-size:12px; color:#1A5276; font-weight:bold; }\n"
        << ".toolbar input { flex:1; min-width:180px; padding:7px 10px; border:1px solid #AED6F1; border-radius:6px; font-size:12px; color:#333; outline:none; }\n"
        << ".toolbar input:focus { border-color:#2E86C1; }\n"
        << ".toolbar select { padding:7px 10px; border:1px solid #AED6F1; border-radius:6px; font-size:12px; background:white; color:#333; cursor:pointer; }\n"
        << ".btn { padding:7px 14px; background:#EBF5FB; border:1px solid #AED6F1; border-radius:6px; font-size:12px; color:#1A5276; cursor:pointer; font-weight:bold; }\n"
        << ".btn:hover { background:#AED6F1; }\n"
        << ".compteur { font-size:11px; color:#888; }\n"
        << ".leg { display:flex; gap:12px; flex-wrap:wrap; margin-bottom:10px; font-size:11px; color:#555; align-items:center; }\n"
        << ".leg-dot { width:12px; height:12px; border-radius:3px; display:inline-block; margin-right:4px; vertical-align:middle; }\n"
        << ".tabs { display:flex; gap:4px; margin-bottom:-1px; }\n"
        << ".tab-btn { padding:8px 20px; border:1px solid #AED6F1; border-bottom:none; border-radius:6px 6px 0 0; background:#EBF5FB; color:#555; cursor:pointer; font-size:12px; font-weight:bold; }\n"
        << ".tab-btn.active { background:#2E86C1; color:white; border-color:#2E86C1; }\n"
        << ".tab-content { display:none; }\n"
        << ".tab-content.active { display:block; }\n"
        << ".tab-wrapper { border:1px solid #AED6F1; border-radius:0 10px 10px 10px; background:white; overflow:hidden; margin-bottom:20px; }\n"
        << "table { width:100%; border-collapse:collapse; font-size:12px; }\n"
        << "thead { background:#EBF5FB; }\n"
        << "th { padding:12px 14px; text-align:left; color:#1A5276; font-size:11px; font-weight:bold; border-bottom:1px solid #AED6F1; }\n"
        << "td { padding:10px 14px; border-bottom:1px solid #F0F0F0; vertical-align:middle; }\n"
        << "tr:last-child td { border-bottom:none; }\n"
        << "tr:hover td { background:#F8FCFF; }\n"
        << ".row-hidden { display:none; }\n"
        << ".id-col { font-weight:bold; color:#1A5276; min-width:200px; }\n"
        << ".badge { display:inline-block; padding:3px 10px; border-radius:12px; font-size:11px; font-weight:bold; }\n"
        << ".badge.couvert    { background:#E8F8F0; color:#27AE60; border:1px solid #27AE60; }\n"
        << ".badge.noncouvert { background:#FDECEA; color:#E74C3C; border:1px solid #E74C3C; }\n"
        << ".badge.lienbrise  { background:#FEF9F0; color:#E67E22; border:1px solid #E67E22; }\n"
        << ".badge.orpheline  { background:#F5EEF8; color:#7D3C98; border:1px solid #7D3C98; }\n"
        << ".vide { color:#AAAAAA; }\n"
        << ".no-result { text-align:center; padding:30px; color:#AAAAAA; font-size:13px; display:none; }\n"
        << ".footer { text-align:right; font-size:10px; color:#AAA; margin-top:20px; border-top:1px solid #DDD; padding-top:8px; }\n"
        << "@media (max-width:768px) { .taux-row { flex-direction:column; } }\n"
        << "</style>\n</head>\n<body>\n"

        << "<div class=\"header\">\n"
        << "  <h1>TraceLink</h1>\n"
        << "  <p>RAPPORT DE TRACABILITE - L2G1 - UNIVERSITE PARIS CITE</p>\n"
        << "</div>\n"

        << "<div class=\"badges\">\n"
        << "  <span class=\"badge-sum\" style=\"background:#27AE60;\">" << nbCouv    << " Couvert(s)</span>\n"
        << "  <span class=\"badge-sum\" style=\"background:#E74C3C;\">" << nbNonCouv << " Non couvert(s)</span>\n"
        << "  <span class=\"badge-sum\" style=\"background:#E67E22;\">" << nbLB      << " Lien(s) brise(s)</span>\n"
        << "  <span class=\"badge-sum\" style=\"background:#7D3C98;\">" << nbOrph    << " Orpheline(s)</span>\n"
        << "</div>\n"

        << "<h2>Taux de couverture</h2>\n"
        << "<div class=\"taux-row\">\n"
        << "  <div class=\"taux-card\">\n"
        << "    <div class=\"taux-label\">Taux SSS vers SRS</div>\n"
        << "    <div class=\"taux-val\" style=\"color:" << couleurTaux(pSSS) << "\">" << pSSS << " %</div>\n"
        << "    <div class=\"prog\"><div class=\"prog-bar\" id=\"barSSS\" style=\"background:" << couleurTaux(pSSS) << "\"></div></div>\n"
        << "  </div>\n"
        << "  <div class=\"taux-card\">\n"
        << "    <div class=\"taux-label\">Taux SRS vers SDD</div>\n"
        << "    <div class=\"taux-val\" style=\"color:" << couleurTaux(pSRS) << "\">" << pSRS << " %</div>\n"
        << "    <div class=\"prog\"><div class=\"prog-bar\" id=\"barSRS\" style=\"background:" << couleurTaux(pSRS) << "\"></div></div>\n"
        << "  </div>\n"
        << "  <div class=\"taux-card\">\n"
        << "    <div class=\"taux-label\">Taux Global</div>\n"
        << "    <div class=\"taux-val\" style=\"color:" << couleurTaux(pGlb) << "\">" << pGlb << " %</div>\n"
        << "    <div class=\"prog\"><div class=\"prog-bar\" id=\"barGlb\" style=\"background:" << couleurTaux(pGlb) << "\"></div></div>\n"
        << "  </div>\n"
        << "</div>\n"

        << "<h2>Matrices de tracabilite</h2>\n"

        // Toolbar : recherche + select statut + bouton Reset (apport Amel)
        << "<div class=\"toolbar\">\n"
        << "  <label>Recherche :</label>\n"
        << "  <input type=\"text\" id=\"recherche\" placeholder=\"Rechercher par ID, mot-cle...\" oninput=\"filtrer()\">\n"
        << "  <label>Statut :</label>\n"
        << "  <select id=\"selectStatut\" onchange=\"filtrer()\">\n"
        << "    <option value=\"tous\">Tous</option>\n"
        << "    <option value=\"couvert\">Couvert</option>\n"
        << "    <option value=\"noncouvert\">Non couvert</option>\n"
        << "    <option value=\"lienbrise\">Lien brise</option>\n"
        << "    <option value=\"orpheline\">Orpheline</option>\n"
        << "  </select>\n"
        << "  <button class=\"btn\" onclick=\"reinitialiser()\">Reinitialiser</button>\n"
        << "  <span class=\"compteur\" id=\"compteur\"></span>\n"
        << "</div>\n"

        // Legende coloree (apport Amel)
        << "<div class=\"leg\">\n"
        << "  <span><span class=\"leg-dot\" style=\"background:#27AE60\"></span>Couvert</span>\n"
        << "  <span><span class=\"leg-dot\" style=\"background:#E74C3C\"></span>Non couvert</span>\n"
        << "  <span><span class=\"leg-dot\" style=\"background:#E67E22\"></span>Lien brise</span>\n"
        << "  <span><span class=\"leg-dot\" style=\"background:#7D3C98\"></span>Orpheline</span>\n"
        << "</div>\n"

        << "<div class=\"tabs\">\n"
        << "  <div class=\"tab-btn active\" onclick=\"showTab('sss', this)\">SSS vers SRS</div>\n"
        << "  <div class=\"tab-btn\" onclick=\"showTab('srs', this)\">SRS vers SDD</div>\n"
        << "</div>\n"
        << "<div class=\"tab-wrapper\">\n"

        << "  <div class=\"tab-content active\" id=\"tab-sss\">\n"
        << "  <table><thead><tr>\n"
        << "    <th style=\"width:35%\">Exigence SSS</th>\n"
        << "    <th style=\"width:20%\">Statut</th>\n"
        << "    <th>Couverte par (SRS)</th>\n"
        << "  </tr></thead><tbody id=\"bodySSS\">\n"
        << lignesSSS
        << "  </tbody></table>\n"
        << "  <div class=\"no-result\" id=\"noSSS\">Aucune exigence ne correspond.</div>\n"
        << "  </div>\n"

        << "  <div class=\"tab-content\" id=\"tab-srs\">\n"
        << "  <table><thead><tr>\n"
        << "    <th style=\"width:35%\">Exigence SRS</th>\n"
        << "    <th style=\"width:20%\">Statut</th>\n"
        << "    <th>Couverte par (SDD)</th>\n"
        << "  </tr></thead><tbody id=\"bodySRS\">\n"
        << lignesSRS
        << "  </tbody></table>\n"
        << "  <div class=\"no-result\" id=\"noSRS\">Aucune exigence ne correspond.</div>\n"
        << "  </div>\n"
        << "</div>\n"

        << "<div class=\"footer\">TraceLink - L2G1 - Genere le "
        << QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm")
        << "</div>\n"

        << "<script>\n"
        << "var tabActif = 'sss';\n"

        // filtrer : recherche sur tout le texte + select statut (apport Amel)
        << "function filtrer() {\n"
        << "  var recherche = document.getElementById('recherche').value.toLowerCase();\n"
        << "  var statut    = document.getElementById('selectStatut').value;\n"
        << "  var idBody = tabActif === 'sss' ? 'bodySSS' : 'bodySRS';\n"
        << "  var noId   = tabActif === 'sss' ? 'noSSS'   : 'noSRS';\n"
        << "  var tbody  = document.getElementById(idBody);\n"
        << "  var rows   = tbody.querySelectorAll('tr');\n"
        << "  var nb = 0;\n"
        << "  rows.forEach(function(tr) {\n"
        << "    var texte  = tr.textContent.toLowerCase();\n"
        << "    var st     = tr.getAttribute('data-statut') || '';\n"
        << "    var matchR = recherche === '' || texte.indexOf(recherche) >= 0;\n"
        << "    var matchS = statut === 'tous' || st === statut;\n"
        << "    if (matchR && matchS) { tr.classList.remove('row-hidden'); nb++; }\n"
        << "    else                  { tr.classList.add('row-hidden'); }\n"
        << "  });\n"
        << "  document.getElementById('compteur').textContent = nb + ' exigence(s) affichee(s)';\n"
        << "  document.getElementById(noId).style.display = nb === 0 ? 'block' : 'none';\n"
        << "}\n"

        // reinitialiser (apport Amel)
        << "function reinitialiser() {\n"
        << "  document.getElementById('recherche').value = '';\n"
        << "  document.getElementById('selectStatut').value = 'tous';\n"
        << "  filtrer();\n"
        << "}\n"

        << "function showTab(tab, btn) {\n"
        << "  tabActif = tab;\n"
        << "  document.querySelectorAll('.tab-btn').forEach(function(b) { b.classList.remove('active'); });\n"
        << "  btn.classList.add('active');\n"
        << "  document.getElementById('tab-sss').classList.toggle('active', tab === 'sss');\n"
        << "  document.getElementById('tab-srs').classList.toggle('active', tab === 'srs');\n"
        << "  filtrer();\n"
        << "}\n"
        << "window.addEventListener('load', function() {\n"
        << "  document.getElementById('barSSS').style.width = '" << pSSS << "%';\n"
        << "  document.getElementById('barSRS').style.width = '" << pSRS << "%';\n"
        << "  document.getElementById('barGlb').style.width = '" << pGlb << "%';\n"
        << "  filtrer();\n"
        << "});\n"
        << "</script>\n"
        << "</body>\n</html>\n";

    fichier.close();
    QMessageBox msg(this);
    msg.setWindowTitle("Export HTML");
    msg.setText("HTML export\u00e9 avec succ\u00e8s ! Ouvrez le fichier dans un navigateur.");
    msg.setStyleSheet("QMessageBox { background-color: white; } QMessageBox QLabel { color: black; }");
    msg.exec();
}


/**
 * \brief Genere un fichier PDF du rapport via QTextDocument et QPrinter.
 *
 * Affiche d'abord une boite de dialogue permettant de choisir quels statuts
 * inclure dans le PDF (couvert, non couvert, lien brise, orpheline).
 * Format A4 paysage, police Arial, lignes alternees, badges colores.
 * Contenu : en-tete, badges resume, taux, matrice SSS x SRS, matrice SRS x SDD,
 *           sections liens brises et orphelines.
 *
 * \author Lamia Arrahmane
 * Apport Amel Kheloul : boite de dialogue filtres, lignes alternees,
 *                       helper badgeSpan, sections lbHtml/orphHtml.
 * Correction v3.1 : QPrinter::HighResolution remplace par QPrinter::ScreenResolution
 *                   + pageRect(QPrinter::Point) pour un rendu lisible sur Windows/MinGW.
 */
void OngletRapport::exporterPDF()
{
    // â”€â”€ Boite de dialogue filtres PDF (apport Amel Kheloul) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    QDialog dlg(this);
    dlg.setWindowTitle("Options d'export PDF");
    dlg.setFixedSize(380, 260);
    dlg.setStyleSheet(R"(
        QDialog { background-color: #EBF5FB; }
        QLabel { color: #1A5276; font-size: 12px; }
        QCheckBox { color: #333; font-size: 12px; padding: 4px; }
        QCheckBox::indicator { width: 16px; height: 16px; }
        QPushButton {
            background-color: #2E86C1; color: white;
            border-radius: 6px; padding: 7px 18px;
            font-size: 12px; font-weight: bold; border: none;
        }
        QPushButton:hover { background-color: #1A5276; }
        QPushButton#btnAnnuler {
            background-color: #EBF5FB; color: #555;
            border: 1px solid #AED6F1;
        }
        QPushButton#btnAnnuler:hover { background-color: #AED6F1; }
    )");

    QVBoxLayout *vl = new QVBoxLayout(&dlg);
    vl->setContentsMargins(20, 16, 20, 16);
    vl->setSpacing(10);

    QLabel *titreDlg = new QLabel("Choisissez les options d'export :", &dlg);
    titreDlg->setStyleSheet("font-size:13px; font-weight:bold; color:#1A5276;");
    vl->addWidget(titreDlg);

    QFrame *sep = new QFrame(&dlg);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#AED6F1;");
    vl->addWidget(sep);

    QLabel *lblStatut = new QLabel("Filtrer par statut de couverture :", &dlg);
    vl->addWidget(lblStatut);

    QCheckBox *chkCouvert    = new QCheckBox("Inclure les exigences couvertes",     &dlg);
    QCheckBox *chkNonCouvert = new QCheckBox("Inclure les exigences non couvertes", &dlg);
    QCheckBox *chkLienBrise  = new QCheckBox("Inclure les liens brises",             &dlg);
    QCheckBox *chkOrpheline  = new QCheckBox("Inclure les orphelines",               &dlg);
    chkCouvert->setChecked(true);
    chkNonCouvert->setChecked(true);
    chkLienBrise->setChecked(true);
    chkOrpheline->setChecked(true);
    vl->addWidget(chkCouvert);
    vl->addWidget(chkNonCouvert);
    vl->addWidget(chkLienBrise);
    vl->addWidget(chkOrpheline);
    vl->addStretch();

    QHBoxLayout *hl = new QHBoxLayout();
    QPushButton *btnOk      = new QPushButton("Exporter", &dlg);
    QPushButton *btnAnnuler = new QPushButton("Annuler",  &dlg);
    btnAnnuler->setObjectName("btnAnnuler");
    hl->addStretch();
    hl->addWidget(btnAnnuler);
    hl->addWidget(btnOk);
    vl->addLayout(hl);
    connect(btnOk,      &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(btnAnnuler, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    bool inclureCouvert    = chkCouvert->isChecked();
    bool inclureNonCouvert = chkNonCouvert->isChecked();
    bool inclureLienBrise  = chkLienBrise->isChecked();
    bool inclureOrpheline  = chkOrpheline->isChecked();

    // â”€â”€ Chemin de sauvegarde â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    QString chemin = QFileDialog::getSaveFileName(
        this, "Exporter en PDF", "rapport_tracabilite.pdf", "PDF (*.pdf)");
    if (chemin.isEmpty()) return;

    int pSSS = qRound(rapportCourant.tauxSSS_SRS * 100);
    int pSRS = qRound(rapportCourant.tauxSRS_SDD * 100);
    int pGlb = qRound(rapportCourant.tauxGlobal  * 100);

    QString nbCouv    = labelBadgeCouverts->text().split(" ")[0];
    QString nbNonCouv = labelBadgeNonCouverts->text().split(" ")[0];
    QString nbLB      = labelBadgeLiensBrises->text().split(" ")[0];
    QString nbOrph    = labelBadgeOrphelines->text().split(" ")[0];

    // Lignes SSS avec filtres et lignes alternees (apport Amel)
    QString lignesSSS;
    int r = 0;
    for (const ExigenceSSS& exig : rapportCourant.exigencesSSS) {
        bool lb = false;
        for (const std::string& l : rapportCourant.liensBrises)
            if (l.find(exig.getId()) != std::string::npos) { lb = true; break; }
        QString bg, st;
        if (lb)                              { bg="#E67E22"; st="Lien brise"; }
        else if (exig.getStatut()==Couverte) { bg="#27AE60"; st="Couvert"; }
        else                                 { bg="#E74C3C"; st="Non couvert"; }

        if (lb           && !inclureLienBrise)  continue;
        if (!lb && exig.getStatut()==Couverte    && !inclureCouvert)    continue;
        if (!lb && exig.getStatut()==NonCouverte && !inclureNonCouvert) continue;

        QStringList srsList;
        for (const ExigenceSRS& s : srsAnalyses) {
            const auto& t = s.getTracabilite();
            if (std::find(t.begin(), t.end(), exig.getId()) != t.end())
                srsList << QString::fromStdString(s.getId());
        }
        QString bgRow = (r++ % 2 == 0) ? "#FFFFFF" : "#F5F7FA";
        lignesSSS +=
            "<tr style='background:" + bgRow + "'>"
                                               "<td style='padding:5px 8px; font-weight:bold; color:#1A5276; border-bottom:1px solid #EEE'>"
            + QString::fromStdString(exig.getId()) + "</td>"
                                                     "<td style='padding:5px 8px; text-align:center; border-bottom:1px solid #EEE'>"
            + badgeSpan(st, bg) + "</td>"
                                  "<td style='padding:5px 8px; color:#555; border-bottom:1px solid #EEE'>"
            + (srsList.isEmpty() ? "-" : srsList.join(", ")) + "</td>"
                                                               "</tr>\n";
    }

    // Lignes SRS avec filtres et lignes alternees (apport Amel)
    QString lignesSRS;
    r = 0;
    for (const ExigenceSRS& exig : srsAnalyses) {
        QStringList sddList;
        for (const ExigenceSDD& d : sddAnalyses) {
            const auto& ref = d.getReferencedSRS();
            if (std::find(ref.begin(), ref.end(), exig.getId()) != ref.end())
                sddList << QString::fromStdString(d.getId());
        }
        bool c = !sddList.isEmpty(), o = exig.getTracabilite().empty(), lb = false;
        for (const std::string& l : rapportCourant.liensBrises)
            if (l.find(exig.getId()) != std::string::npos) { lb = true; break; }
        QString bg, st;
        if (lb)     { bg="#E67E22"; st="Lien brise"; }
        else if (o) { bg="#7D3C98"; st="Orpheline"; }
        else if (c) { bg="#27AE60"; st="Couvert"; }
        else        { bg="#E74C3C"; st="Non couvert"; }

        if (lb           && !inclureLienBrise)  continue;
        if (o            && !inclureOrpheline)  continue;
        if (!lb && !o &&  c && !inclureCouvert)    continue;
        if (!lb && !o && !c && !inclureNonCouvert) continue;

        QString bgRow = (r++ % 2 == 0) ? "#FFFFFF" : "#F5F7FA";
        lignesSRS +=
            "<tr style='background:" + bgRow + "'>"
                                               "<td style='padding:5px 8px; font-weight:bold; color:#1A5276; border-bottom:1px solid #EEE'>"
            + QString::fromStdString(exig.getId()) + "</td>"
                                                     "<td style='padding:5px 8px; text-align:center; border-bottom:1px solid #EEE'>"
            + badgeSpan(st, bg) + "</td>"
                                  "<td style='padding:5px 8px; color:#555; border-bottom:1px solid #EEE'>"
            + (sddList.isEmpty() ? "-" : sddList.join(", ")) + "</td>"
                                                               "</tr>\n";
    }

    // Sections liens brises et orphelines dans le PDF (apport Amel)
    QString lbHtml, orphHtml;
    if (!rapportCourant.liensBrises.empty()) {
        lbHtml = "<h3 style='color:#E67E22; margin:16px 0 6px'>Liens brises</h3>";
        for (const std::string& l : rapportCourant.liensBrises)
            lbHtml += "<div style='background:#FEF9F0; border:1px solid #F0B27A; border-radius:4px;"
                      "padding:5px 10px; margin:3px 0; color:#A04000; font-size:11px'>"
                      + QString::fromStdString(l) + "</div>\n";
    }
    if (!orphelinesAnalyses.empty()) {
        orphHtml = "<h3 style='color:#7D3C98; margin:16px 0 6px'>Exigences orphelines</h3>";
        for (const std::string& o : orphelinesAnalyses)
            orphHtml += "<div style='background:#F5EEF8; border:1px solid #A569BD; border-radius:4px;"
                        "padding:5px 10px; margin:3px 0; color:#7D3C98; font-size:11px'>"
                        + QString::fromStdString(o) + "</div>\n";
    }

    // Construction du HTML complet pour le PDF
    QString html =
        "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
        "<style>"
        "body { font-family:Arial,sans-serif; color:#333; margin:0; padding:16px; font-size:11px; }"
        "h1 { color:#1A5276; font-size:17px; border-bottom:2px solid #AED6F1; padding-bottom:7px; margin-bottom:12px; }"
        "h2 { color:#1A5276; font-size:12px; margin:14px 0 6px; }"
        ".badges { margin-bottom:12px; }"
        ".badge { display:inline-block; padding:2px 8px; border-radius:8px; color:white; font-size:10px; font-weight:bold; margin-right:4px; }"
        ".taux-row { display:flex; gap:10px; margin-bottom:14px; }"
        ".tc { flex:1; background:#EBF5FB; border-radius:6px; padding:8px 12px; border:1px solid #AED6F1; }"
        ".tl { font-size:9px; color:#555; margin-bottom:3px; }"
        ".tv { font-size:18px; font-weight:bold; }"
        "table { border-collapse:collapse; width:100%; margin-bottom:14px; font-size:10px; }"
        "th { background:#1A5276; color:white; padding:7px 8px; text-align:left; }"
        "th:nth-child(2) { text-align:center; width:100px; }"
        "td:nth-child(2) { text-align:center; }"
        "</style></head><body>"
        "<h1>Rapport de tracabilite - TraceLink</h1>"
        "<div class='badges'>"
        + QString("<span class='badge' style='background:#27AE60'>") + nbCouv    + " Couvert</span>"
        + "<span class='badge' style='background:#E74C3C'>"          + nbNonCouv + " Non couvert</span>"
        + "<span class='badge' style='background:#E67E22'>"          + nbLB      + " Lien brise</span>"
        + "<span class='badge' style='background:#7D3C98'>"          + nbOrph    + " Orpheline</span>"
        + "</div>"
          "<div class='taux-row'>"
          "<div class='tc'><div class='tl'>Taux SSS vers SRS</div>"
          "<div class='tv' style='color:" + couleurTaux(pSSS) + "'>" + QString::number(pSSS) + " %</div></div>"
                                                             "<div class='tc'><div class='tl'>Taux SRS vers SDD</div>"
                                                             "<div class='tv' style='color:" + couleurTaux(pSRS) + "'>" + QString::number(pSRS) + " %</div></div>"
                                                             "<div class='tc'><div class='tl'>Taux global</div>"
                                                             "<div class='tv' style='color:" + couleurTaux(pGlb) + "'>" + QString::number(pGlb) + " %</div></div>"
                                                             "</div>"
                                                             "<h2>Matrice SSS vers SRS</h2>"
                                                             "<table><thead><tr>"
                                                             "<th>Exigence SSS</th><th>Statut</th><th>Couverte par (SRS)</th>"
                                                             "</tr></thead><tbody>" + lignesSSS + "</tbody></table>"
                      "<h2>Matrice SRS vers SDD</h2>"
                      "<table><thead><tr>"
                      "<th>Exigence SRS</th><th>Statut</th><th>Couverte par (SDD)</th>"
                      "</tr></thead><tbody>" + lignesSRS + "</tbody></table>"
        + lbHtml + orphHtml
        + "<p style='text-align:right; font-size:9px; color:#AAA; margin-top:14px;"
          "border-top:1px solid #DDD; padding-top:6px'>"
          "TraceLink - Genere le "
        + QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm") + "</p>"
                                                                      "</body></html>";

    // â”€â”€ CORRECTION v3.1 : ScreenResolution + Point â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    // PROBLEME : QPrinter::HighResolution (1200 dpi) faisait calculer a
    //            QTextDocument des dimensions de page enormes en DevicePixel,
    //            ce qui produisait un texte microscopique dans le PDF.
    // SOLUTION : QPrinter::ScreenResolution (96 dpi) donne des dimensions
    //            raisonnables, et pageRect(QPrinter::Point) utilise les points
    //            typographiques (72 pt = 1 pouce), l'unite naturelle de
    //            QTextDocument pour le rendu HTML -> PDF.
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(chemin);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);

    QTextDocument doc;
    doc.setHtml(html);
    // IMPORTANT : Point (pas DevicePixel) pour un rendu lisible
    doc.setPageSize(printer.pageRect(QPrinter::Point).size());
    doc.print(&printer);

    QMessageBox msg(this);
    msg.setWindowTitle("Export PDF");
    msg.setText("Fichier PDF export\u00e9 avec succ\u00e8s !");
    msg.setStyleSheet("QMessageBox { background-color: white; } QMessageBox QLabel { color: black; }");
    msg.exec();
}
