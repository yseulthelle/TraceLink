/**
 * @file mainwindow.cpp
 * @brief Implémentation de la fenêtre principale de TraceLink — Module 3.
 * @date 26/04/2026
 * @version 8.1 — Préfixe configurable par fichier + label Fichiers sélectionnés
 *                + item vide par défaut + design bouton réinitialiser
 */

#include "mainwindow.h"
#include <QFileDialog>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileInfo>
#include <QTextEdit>
#include <QMessageBox>
#include <QListWidget>
#include <QProgressBar>
#include <QTimer>
#include <algorithm>
#include <iostream>
#include <sstream>

#include "ExigenceSSS.h"
#include "ExigenceSRS.h"
#include "ExigenceSDD.h"

// ─── Constructeur ─────────────────────────────────────────────────────────────
/**
 * @brief Constructeur de la fenêtre principale.
 * Initialise la fenêtre, applique le style global, construit les 3 onglets,
 * connecte les signaux du gestionnaire de filtres.
 * @param parent Widget parent Qt.
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("TraceLink - Outil de Traçabilité");
    resize(1100, 700);

    setStyleSheet(R"(
        QMainWindow { background-color: #EBF5FB; }
        QTabWidget::pane { border: none; background-color: #EBF5FB; }
        QTabBar::tab {
            background: #FFFFFF; color: #555555;
            padding: 8px 20px; border-radius: 4px;
            margin-right: 4px; border: 1px solid #DDDDDD;
        }
        QTabBar::tab:selected { background: #2E86C1; color: white; font-weight: bold; border: none; }
        QGroupBox {
            color: #333333; border: 1px solid #DDDDDD;
            border-radius: 8px; margin-top: 10px;
            padding: 10px; font-size: 11px; background-color: #FFFFFF;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
        QLineEdit {
            background-color: #FFFFFF; color: #333333;
            border: 1px solid #DDDDDD; border-radius: 4px; padding: 6px;
        }
        QPushButton {
            background-color: #FFFFFF; color: #555555;
            border-radius: 6px; padding: 6px 14px;
            font-size: 11px; border: 1px solid #DDDDDD;
        }
        QPushButton:hover { background-color: #E8F0FE; color: #2E86C1; }
        QListWidget {
            background-color: #FFFFFF; border: 1px solid #DDDDDD;
            border-radius: 6px; font-size: 10px;
        }
        QProgressBar { background-color: #EEEEEE; border-radius: 4px; border: none; }
        QProgressBar::chunk { background-color: #2E86C1; border-radius: 4px; }
    )");

    tabWidget = new QTabWidget(this);
    setCentralWidget(tabWidget);

    construireOngletImport();
    construireOngletGraphe();
    construireOngletRapport();

    gestionnaireFiltres = new GestionnaireFiltresGUI();
    connect(gestionnaireFiltres, &GestionnaireFiltresGUI::filtreModifie,
            ongletRapport, &OngletRapport::appliquerFiltre);
    connect(gestionnaireFiltres, &GestionnaireFiltresGUI::filtreModifie,
            ongletGraphe, &VueGrapheTracabilite::appliquerFiltre);


    tabWidget->addTab(ongletImport,           "📂 Import des fichiers");
    tabWidget->addTab(ongletGrapheConteneur,  "🔗 Graphe de traçabilité");
    tabWidget->addTab(ongletRapportConteneur, "📊 Rapport & Taux");



    // Boutons vue directe/indirecte dans le coin de la barre des onglets
    QWidget *coinBoutons = new QWidget();
    QHBoxLayout *layoutCoin = new QHBoxLayout(coinBoutons);
    layoutCoin->setContentsMargins(0, 0, 8, 0);
    layoutCoin->setSpacing(6);

    QPushButton *btnVueDirecte   = new QPushButton("📊  Vue directe", coinBoutons);
    QPushButton *btnVueIndirecte = new QPushButton("🔗  Vue indirecte", coinBoutons);

    // Cacher les boutons par défaut
    coinBoutons->setVisible(false);

    // Afficher/cacher selon l'onglet actif
    connect(tabWidget, &QTabWidget::currentChanged, this, [this, coinBoutons](int index) {
        coinBoutons->setVisible(index == 1); // 1 = onglet graphe
    });

    btnVueDirecte->setFixedHeight(28);
    btnVueIndirecte->setFixedHeight(28);

    btnVueDirecte->setStyleSheet(R"(
        QPushButton {
            background-color: #1A6FA8; color: white;
            border-radius: 6px; padding: 4px 14px;
            font-size: 11px; font-weight: bold; border: none;
        }
        QPushButton:hover { background-color: #0D2B45; }
    )");
    btnVueIndirecte->setStyleSheet(R"(
        QPushButton {
            background-color: #FFFFFF; color: #555555;
            border-radius: 6px; padding: 4px 14px;
            font-size: 11px; border: 1px solid #CCDDEE;
        }
        QPushButton:hover { background-color: #EBF5FB; color: #1A6FA8; }
    )");

    layoutCoin->addWidget(btnVueDirecte);
    layoutCoin->addWidget(btnVueIndirecte);
    tabWidget->setCornerWidget(coinBoutons, Qt::TopRightCorner);

    // Connecter aux slots du graphe
    connect(btnVueDirecte, &QPushButton::clicked, this, [this, btnVueDirecte, btnVueIndirecte]() {
        ongletGraphe->setVueDirecte();
        btnVueDirecte->setStyleSheet(R"(
            QPushButton { background-color: #1A6FA8; color: white; border-radius: 6px; padding: 4px 14px; font-size: 11px; font-weight: bold; border: none; }
            QPushButton:hover { background-color: #0D2B45; }
        )");
        btnVueIndirecte->setStyleSheet(R"(
            QPushButton { background-color: #FFFFFF; color: #555555; border-radius: 6px; padding: 4px 14px; font-size: 11px; border: 1px solid #CCDDEE; }
            QPushButton:hover { background-color: #EBF5FB; color: #1A6FA8; }
        )");
    });
    connect(btnVueIndirecte, &QPushButton::clicked, this, [this, btnVueDirecte, btnVueIndirecte]() {
        ongletGraphe->setVueIndirecte();
        btnVueIndirecte->setStyleSheet(R"(
            QPushButton { background-color: #3D3D3D; color: white; border-radius: 6px; padding: 4px 14px; font-size: 11px; font-weight: bold; border: none; }
            QPushButton:hover { background-color: #222222; }
        )");
        btnVueDirecte->setStyleSheet(R"(
            QPushButton { background-color: #FFFFFF; color: #555555; border-radius: 6px; padding: 4px 14px; font-size: 11px; border: 1px solid #CCDDEE; }
            QPushButton:hover { background-color: #EBF5FB; color: #1A6FA8; }
        )");
    });

    connect(btnLancer, &QPushButton::clicked, this, &MainWindow::lancerAnalyse);
}

/**
 * @brief Destructeur — libère le thread et le moteur si encore actifs.
 */
MainWindow::~MainWindow()
{
    if (threadManager) { threadManager->attendreFinAnalyse(); delete threadManager; }
    if (moteurAnalyse) delete moteurAnalyse;
}

// ─── ajouterItemVide ──────────────────────────────────────────────────────────
/**
 * @brief Ajoute l'item "Aucun fichier sélectionné" par défaut dans une liste.
 * L'item est grisé et non sélectionnable.
 * @param liste La QListWidget à initialiser.
 */
void MainWindow::ajouterItemVide(QListWidget *liste)
{
    QListWidgetItem *vide = new QListWidgetItem("📭  Aucun fichier sélectionné");
    vide->setForeground(QColor("#AAAAAA"));
    vide->setFlags(Qt::NoItemFlags);
    liste->addItem(vide);
}

// ─── creerChampsPrefixes ──────────────────────────────────────────────────────
/**
 * @brief Crée un champ préfixe par fichier dans le layout donné.
 * @param chemins     Liste des chemins de fichiers importés.
 * @param layout      Layout vertical où ajouter les champs.
 * @param map         Map chemin → QLineEdit à remplir.
 * @param placeholder Texte indicatif dans le champ.
 */
void MainWindow::creerChampsPrefixes(const QStringList &chemins,
                                     QVBoxLayout *layout,
                                     QMap<QString, QLineEdit*> &map,
                                     const QString &placeholder)
{
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    map.clear();

    QString styleChamp = R"(
        QLineEdit {
            font-size: 10px; background-color: #F4F9FF;
            border: 1px solid #C8DDEF; border-radius: 6px;
            padding: 4px 8px; color: #0D2B45;
        }
        QLineEdit:focus { border: 1px solid #1A6FA8; background-color: #EBF5FB; }
    )";

    QLabel *titre = new QLabel("⚙  Préfixes des identifiants");
    titre->setStyleSheet("font-size: 10px; font-weight: bold; color: #0D2B45; padding: 4px 0; border-bottom: 1px solid #C8DDEF;");
    layout->addWidget(titre);

    QLabel *info = new QLabel("Indiquez par quoi commencent les IDs dans chaque fichier");
    info->setStyleSheet("color: #7A9AB8; font-size: 9px; padding: 2px 0;");
    info->setWordWrap(true);
    layout->addWidget(info);

    for (const QString &chemin : chemins) {
        QWidget *row = new QWidget();
        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 2, 0, 2);
        rowLayout->setSpacing(6);

        QLabel *lbl = new QLabel(QFileInfo(chemin).fileName());
        lbl->setStyleSheet("font-size: 9px; color: #4A7A9B;");
        lbl->setFixedWidth(110);
        lbl->setWordWrap(true);

        QLineEdit *champ = new QLineEdit();
        champ->setPlaceholderText(placeholder);
        champ->setStyleSheet(styleChamp);

        rowLayout->addWidget(lbl);
        rowLayout->addWidget(champ);
        layout->addWidget(row);
        map[chemin] = champ;
    }
}

// ─── construireOngletImport ───────────────────────────────────────────────────
/**
 * @brief Construit l'onglet Import en 3 colonnes :
 *        Gauche = sélection fichiers + préfixes par fichier
 *        Milieu = terminal + progression
 *        Droite = aperçu exigences
 */
void MainWindow::construireOngletImport()
{
    ongletImport = new QWidget();
    ongletImport->setStyleSheet("background-color: #EEF6FF;");

    QHBoxLayout *layoutPrincipal = new QHBoxLayout(ongletImport);
    layoutPrincipal->setContentsMargins(14, 14, 14, 14);
    layoutPrincipal->setSpacing(14);

    // ══ COLONNE GAUCHE ══
    QWidget *colonneGauche = new QWidget();
    colonneGauche->setFixedWidth(310);
    colonneGauche->setStyleSheet("background-color: transparent;");
    QVBoxLayout *layoutGauche = new QVBoxLayout(colonneGauche);
    layoutGauche->setContentsMargins(0, 0, 0, 0);
    layoutGauche->setSpacing(10);

    QString styleListeFichiers = R"(
        QListWidget {
            background-color: #F4F9FF; border: 1px solid #C8DDEF;
            border-radius: 6px; font-size: 9px; color: #1A3A5C;
        }
        QListWidget::item { padding: 3px 8px; color: #1A3A5C; border-bottom: 1px solid #E0EEF8; }
        QListWidget::item:selected { background-color: #D0E8F8; color: #0D2B45; }
    )";
    QString styleChampFichier = R"(
        QLineEdit {
            background-color: #F4F9FF; color: #1A3A5C;
            border: 1px solid #C8DDEF; border-radius: 6px;
            padding: 6px 10px; font-size: 10px;
        }
    )";
    QString styleBtnParcourir = R"(
        QPushButton {
            background-color: #1A6FA8; color: white;
            border-radius: 6px; padding: 6px 14px;
            font-size: 10px; font-weight: bold; border: none;
        }
        QPushButton:hover { background-color: #0D2B45; }
    )";
    QString styleLabelListe = "color: #4A7A9B; font-size: 9px; font-weight: bold; padding: 2px 0px;";

    // ── Onglets SSS / SRS / SDD ──
    ongletsFichiers = new QTabWidget();
    ongletsFichiers->setStyleSheet(R"(
        QTabWidget::pane { border: 1px solid #C8DDEF; border-radius: 10px; background-color: #FFFFFF; padding: 10px; }
        QTabBar::tab { background: #EEF6FF; color: #4A7A9B; padding: 7px 18px; border-radius: 6px; margin-right: 4px; border: 1px solid #C8DDEF; font-size: 11px; font-weight: bold; }
        QTabBar::tab:selected { background: #0D2B45; color: white; border: none; }
        QTabBar::tab:hover:!selected { background: #D0E8F8; color: #0D2B45; }
    )");

    // ── Tab SSS ──
    QWidget *tabSSS = new QWidget(); tabSSS->setStyleSheet("background-color: transparent;");
    QVBoxLayout *lSSS = new QVBoxLayout(tabSSS); lSSS->setSpacing(6);
    champSSS = new QLineEdit(); champSSS->setPlaceholderText("Aucun fichier sélectionné..."); champSSS->setReadOnly(true); champSSS->setStyleSheet(styleChampFichier);
    btnParcourirSSS = new QPushButton("📁  Parcourir"); btnParcourirSSS->setStyleSheet(styleBtnParcourir);
    badgeSSS = new QLabel(""); badgeSSS->setAlignment(Qt::AlignCenter); badgeSSS->setTextFormat(Qt::RichText);
    erreurSSS = new QLabel(""); erreurSSS->setStyleSheet("color: #E74C3C; font-size: 9px;"); erreurSSS->setWordWrap(true);
    QLabel *labelFichiersSSS = new QLabel("Fichiers sélectionnés :"); labelFichiersSSS->setStyleSheet(styleLabelListe);
    listeSSS = new QListWidget(); listeSSS->setFixedHeight(65); listeSSS->setStyleSheet(styleListeFichiers);
    ajouterItemVide(listeSSS);
    framesPrefixesSSS = new QFrame(); framesPrefixesSSS->setStyleSheet("QFrame { background: transparent; border: none; }");
    layoutPrefixesSSS = new QVBoxLayout(framesPrefixesSSS); layoutPrefixesSSS->setContentsMargins(0, 4, 0, 0); layoutPrefixesSSS->setSpacing(4);
    lSSS->addWidget(champSSS); lSSS->addWidget(btnParcourirSSS); lSSS->addWidget(badgeSSS); lSSS->addWidget(erreurSSS);
    lSSS->addWidget(labelFichiersSSS); lSSS->addWidget(listeSSS); lSSS->addWidget(framesPrefixesSSS); lSSS->addStretch();

    // ── Tab SRS ──
    QWidget *tabSRS = new QWidget(); tabSRS->setStyleSheet("background-color: transparent;");
    QVBoxLayout *lSRS = new QVBoxLayout(tabSRS); lSRS->setSpacing(6);
    champSRS = new QLineEdit(); champSRS->setPlaceholderText("Aucun fichier sélectionné..."); champSRS->setReadOnly(true); champSRS->setStyleSheet(styleChampFichier);
    btnParcourirSRS = new QPushButton("📁  Parcourir"); btnParcourirSRS->setStyleSheet(styleBtnParcourir);
    badgeSRS = new QLabel(""); badgeSRS->setAlignment(Qt::AlignCenter); badgeSRS->setTextFormat(Qt::RichText);
    erreurSRS = new QLabel(""); erreurSRS->setStyleSheet("color: #E74C3C; font-size: 9px;"); erreurSRS->setWordWrap(true);
    QLabel *labelFichiersSRS = new QLabel("Fichiers sélectionnés :"); labelFichiersSRS->setStyleSheet(styleLabelListe);
    listeSRS = new QListWidget(); listeSRS->setFixedHeight(65); listeSRS->setStyleSheet(styleListeFichiers);
    ajouterItemVide(listeSRS);
    framesPrefixesSRS = new QFrame(); framesPrefixesSRS->setStyleSheet("QFrame { background: transparent; border: none; }");
    layoutPrefixesSRS = new QVBoxLayout(framesPrefixesSRS); layoutPrefixesSRS->setContentsMargins(0, 4, 0, 0); layoutPrefixesSRS->setSpacing(4);
    lSRS->addWidget(champSRS); lSRS->addWidget(btnParcourirSRS); lSRS->addWidget(badgeSRS); lSRS->addWidget(erreurSRS);
    lSRS->addWidget(labelFichiersSRS); lSRS->addWidget(listeSRS); lSRS->addWidget(framesPrefixesSRS); lSRS->addStretch();

    // ── Tab SDD ──
    QWidget *tabSDD = new QWidget(); tabSDD->setStyleSheet("background-color: transparent;");
    QVBoxLayout *lSDD = new QVBoxLayout(tabSDD); lSDD->setSpacing(6);
    champSDD = new QLineEdit(); champSDD->setPlaceholderText("Aucun fichier sélectionné..."); champSDD->setReadOnly(true); champSDD->setStyleSheet(styleChampFichier);
    btnParcourirSDD = new QPushButton("📁  Parcourir"); btnParcourirSDD->setStyleSheet(styleBtnParcourir);
    badgeSDD = new QLabel(""); badgeSDD->setAlignment(Qt::AlignCenter); badgeSDD->setTextFormat(Qt::RichText);
    erreurSDD = new QLabel(""); erreurSDD->setStyleSheet("color: #E74C3C; font-size: 9px;"); erreurSDD->setWordWrap(true);
    QLabel *labelFichiersSDD = new QLabel("Fichiers sélectionnés :"); labelFichiersSDD->setStyleSheet(styleLabelListe);
    listeSDD = new QListWidget(); listeSDD->setFixedHeight(65); listeSDD->setStyleSheet(styleListeFichiers);
    ajouterItemVide(listeSDD);
    framesPrefixesSDD = new QFrame(); framesPrefixesSDD->setStyleSheet("QFrame { background: transparent; border: none; }");
    layoutPrefixesSDD = new QVBoxLayout(framesPrefixesSDD); layoutPrefixesSDD->setContentsMargins(0, 4, 0, 0); layoutPrefixesSDD->setSpacing(4);
    lSDD->addWidget(champSDD); lSDD->addWidget(btnParcourirSDD); lSDD->addWidget(badgeSDD); lSDD->addWidget(erreurSDD);
    lSDD->addWidget(labelFichiersSDD); lSDD->addWidget(listeSDD); lSDD->addWidget(framesPrefixesSDD); lSDD->addStretch();

    ongletsFichiers->addTab(tabSSS, "⏳ SSS");
    ongletsFichiers->addTab(tabSRS, "⏳ SRS");
    ongletsFichiers->addTab(tabSDD, "⏳ SDD");

    // ── Label statut ──
    labelStatut = new QLabel("⏳  En attente de fichiers SSS et SRS...");
    labelStatut->setAlignment(Qt::AlignCenter); labelStatut->setWordWrap(true);
    labelStatut->setStyleSheet("color: #7A9AB8; font-size: 10px; padding: 7px; background-color: #FFFFFF; border-radius: 8px; border: 1px solid #C8DDEF;");

    // ── Bouton Lancer ──
    btnLancer = new QPushButton("▶   Lancer l'analyse");
    btnLancer->setEnabled(false); btnLancer->setMinimumHeight(42);
    btnLancer->setStyleSheet(R"(
        QPushButton { background-color: #C8DDEF; color: #8AACCC; border-radius: 10px; font-size: 12px; font-weight: bold; border: none; }
        QPushButton:!disabled { background-color: #0D2B45; color: white; border: none; }
        QPushButton:!disabled:hover { background-color: #1A6FA8; }
    )");

    // ── Bouton Réinitialiser (style copine : rouge dès le départ) ──
    btnReinitialiser = new QPushButton("↺   Réinitialiser");
    btnReinitialiser->setStyleSheet(R"(
        QPushButton {
            background-color: #FFF5F5; color: #E74C3C;
            border-radius: 8px; font-size: 10px;
            border: 1px solid #E74C3C; padding: 5px;
        }
        QPushButton:hover { background-color: #FADBD8; color: #C0392B; border-color: #C0392B; }
    )");

    layoutGauche->addWidget(ongletsFichiers);
    layoutGauche->addWidget(labelStatut);
    layoutGauche->addWidget(btnLancer);
    layoutGauche->addWidget(btnReinitialiser);

    // ══ COLONNE MILIEU ══
    QWidget *colonneMilieu = new QWidget(); colonneMilieu->setStyleSheet("background-color: transparent;");
    QVBoxLayout *layoutMilieu = new QVBoxLayout(colonneMilieu); layoutMilieu->setContentsMargins(0, 0, 0, 0); layoutMilieu->setSpacing(8);

    QFrame *frameTerminalHeader = new QFrame(); frameTerminalHeader->setFixedHeight(34);
    frameTerminalHeader->setStyleSheet("QFrame { background-color: #0D2B45; border-radius: 8px 8px 0px 0px; border: none; }");
    QHBoxLayout *layoutTerminalHeader = new QHBoxLayout(frameTerminalHeader); layoutTerminalHeader->setContentsMargins(12, 0, 12, 0);
    QLabel *titreTerminal = new QLabel("● Journal d'analyse"); titreTerminal->setStyleSheet("color: #7EC8E3; font-size: 11px; font-weight: bold;");
    QLabel *dots = new QLabel("🔴 🟡 🟢"); dots->setStyleSheet("font-size: 10px;");
    layoutTerminalHeader->addWidget(dots); layoutTerminalHeader->addWidget(titreTerminal, 1, Qt::AlignCenter);

    terminal = new QTextEdit(); terminal->setReadOnly(true);
    terminal->setStyleSheet(R"(QTextEdit { background-color: #0A1628; color: #C8E6F5; font-family: "Courier New"; font-size: 10px; border-radius: 0px 0px 10px 10px; border: 1px solid #1A3A5C; border-top: none; padding: 10px; })");

    QLabel *titreProgress = new QLabel("Progression de l'analyse"); titreProgress->setStyleSheet("font-size: 10px; font-weight: bold; color: #4A7A9B;");

    barreProgression = new QProgressBar(); barreProgression->setRange(0, 100); barreProgression->setValue(0); barreProgression->setTextVisible(true); barreProgression->setFixedHeight(20);
    barreProgression->setStyleSheet(R"(
        QProgressBar { background-color: #D0E8F8; border-radius: 10px; border: none; text-align: center; color: white; font-size: 10px; font-weight: bold; }
        QProgressBar::chunk { background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0D2B45, stop:1 #1A6FA8); border-radius: 10px; }
    )");

    layoutMilieu->addWidget(frameTerminalHeader); layoutMilieu->addWidget(terminal, 1); layoutMilieu->addWidget(titreProgress); layoutMilieu->addWidget(barreProgression);

    // ══ COLONNE DROITE ══
    QWidget *colonneDroite = new QWidget(); colonneDroite->setFixedWidth(270); colonneDroite->setStyleSheet("background-color: transparent;");
    QVBoxLayout *layoutDroite = new QVBoxLayout(colonneDroite); layoutDroite->setContentsMargins(0, 0, 0, 0); layoutDroite->setSpacing(8);

    QFrame *frameApercuHeader = new QFrame(); frameApercuHeader->setFixedHeight(34);
    frameApercuHeader->setStyleSheet("QFrame { background-color: #0D2B45; border-radius: 10px 10px 0px 0px; border: none; }");
    QHBoxLayout *layoutApercuHeader = new QHBoxLayout(frameApercuHeader); layoutApercuHeader->setContentsMargins(12, 0, 12, 0);
    QLabel *titreApercu = new QLabel("📋  Aperçu des exigences"); titreApercu->setStyleSheet("color: #7EC8E3; font-size: 11px; font-weight: bold;");
    layoutApercuHeader->addWidget(titreApercu, 1, Qt::AlignCenter);

    champRechercheId = new QLineEdit(); champRechercheId->setPlaceholderText("🔍  Rechercher par ID...");
    champRechercheId->setStyleSheet(R"(QLineEdit { background-color: #FFFFFF; color: #0D2B45; border: 1px solid #C8DDEF; border-radius: 0px; padding: 6px 10px; font-size: 10px; border-top: none; } QLineEdit:focus { border-color: #1A6FA8; background-color: #F4F9FF; })");

    listeExigences = new QListWidget();
    listeExigences->setStyleSheet(R"(QListWidget { background-color: #FFFFFF; border: 1px solid #C8DDEF; border-radius: 0px 0px 10px 10px; font-size: 10px; border-top: none; } QListWidget::item { padding: 6px 10px; border-bottom: 1px solid #EEF6FF; color: #1A3A5C; } QListWidget::item:selected { background-color: #D0E8F8; color: #0D2B45; } QListWidget::item:hover { background-color: #F4F9FF; })");

    labelNbExigences = new QLabel("0 exigence(s) détectée(s)"); labelNbExigences->setAlignment(Qt::AlignCenter);
    labelNbExigences->setStyleSheet("color: #7A9AB8; font-size: 10px; padding: 6px; background-color: #FFFFFF; border: 1px solid #C8DDEF; border-top: none; border-radius: 0px 0px 8px 8px;");

    layoutDroite->addWidget(frameApercuHeader); layoutDroite->addWidget(champRechercheId); layoutDroite->addWidget(listeExigences, 1); layoutDroite->addWidget(labelNbExigences);

    // ══ Assemblage ══
    layoutPrincipal->addWidget(colonneGauche); layoutPrincipal->addWidget(colonneMilieu, 1); layoutPrincipal->addWidget(colonneDroite);

    connect(btnParcourirSSS,  &QPushButton::clicked,   this, &MainWindow::parcourirSSS);
    connect(btnParcourirSRS,  &QPushButton::clicked,   this, &MainWindow::parcourirSRS);
    connect(btnParcourirSDD,  &QPushButton::clicked,   this, &MainWindow::parcourirSDD);
    connect(btnReinitialiser, &QPushButton::clicked,   this, &MainWindow::reinitialiser);
    connect(champRechercheId, &QLineEdit::textChanged, this, &MainWindow::filtrerApercu);
}

// ─── construireOngletGraphe ───────────────────────────────────────────────────
/**
 * @brief Construit l'onglet Graphe de traçabilité.
 */
void MainWindow::construireOngletGraphe()
{
    ongletGrapheConteneur = new QWidget();
    ongletGrapheConteneur->setStyleSheet("background-color: #EBF5FB;");
    QVBoxLayout *layout = new QVBoxLayout(ongletGrapheConteneur);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    ongletGraphe = new VueGrapheTracabilite();

    QScrollArea *scrollArea = new QScrollArea(ongletGrapheConteneur);
    scrollArea->setWidget(ongletGraphe);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setStyleSheet(R"(
        QScrollArea { border: none; background-color: #EBF5FB; }
        QScrollBar:vertical {
            background: #EBF5FB; width: 8px; border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: #AED6F1; border-radius: 4px; min-height: 20px;
        }
        QScrollBar::handle:vertical:hover { background: #2E86C1; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
    )");

    layout->addWidget(scrollArea, 1); // ← prend tout l'espace disponible
    layout->addWidget(ongletGraphe->getPanneauInfo()); // ← barre fixe en bas
}

// ─── construireOngletRapport ──────────────────────────────────────────────────
/**
 * @brief Construit l'onglet Rapport & Taux.
 */
void MainWindow::construireOngletRapport()
{
    ongletRapportConteneur = new QWidget(); ongletRapportConteneur->setStyleSheet("background-color: #EBF5FB;");
    QVBoxLayout *layout = new QVBoxLayout(ongletRapportConteneur); layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);
    ongletRapport = new OngletRapport(ongletRapportConteneur);
    layout->addWidget(ongletRapport);
}

// ─── mettreAJourOngletFichier ─────────────────────────────────────────────────
/**
 * @brief Met à jour le titre de l'onglet SSS/SRS/SDD.
 * @param index 0=SSS, 1=SRS, 2=SDD
 * @param etat  "accepte" → ✓ | "erreur" → ✗ | autre → ⏳
 */
void MainWindow::mettreAJourOngletFichier(int index, const QString &etat)
{
    QStringList noms = {"SSS", "SRS", "SDD"};
    QString texte;
    if (etat == "accepte")     texte = "✓ " + noms[index];
    else if (etat == "erreur") texte = "✗ " + noms[index];
    else                       texte = "⏳ " + noms[index];
    ongletsFichiers->setTabText(index, texte);
}

// ─── formatAccepte ────────────────────────────────────────────────────────────
/**
 * @brief Vérifie si l'extension du fichier est acceptée.
 * @param chemin Chemin complet du fichier.
 * @return true si .docx/.doc/.csv/.xls/.xlsx/.txt
 */
bool MainWindow::formatAccepte(const QString &chemin)
{
    QString ext = QFileInfo(chemin).suffix().toLower();
    return ext == "docx" || ext == "doc" || ext == "csv"
           || ext == "xls" || ext == "xlsx" || ext == "txt";
}

// ─── parcourirSSS ─────────────────────────────────────────────────────────────
/**
 * @brief Sélection multiple de fichiers SSS.
 */
void MainWindow::parcourirSSS()
{
    QStringList chemins = QFileDialog::getOpenFileNames(
        this, "Sélectionner les fichiers SSS", "",
        "Fichiers supportés (*.docx *.doc *.csv *.xls *.xlsx *.txt);;Tous (*)");
    if (!chemins.isEmpty()) {
        cheminsSSS = chemins; champSSS->setText(chemins.join(" | ")); listeSSS->clear();
        bool tousValides = true;
        for (const QString &c : chemins) {
            QFileInfo info(c);
            if (formatAccepte(c)) { QListWidgetItem *it = new QListWidgetItem("📄 " + info.fileName()); it->setForeground(QColor("#333333")); listeSSS->addItem(it); }
            else { QListWidgetItem *it = new QListWidgetItem("⚠ " + info.fileName()); it->setForeground(QColor("#E74C3C")); listeSSS->addItem(it); tousValides = false; }
        }
        if (tousValides) {
            badgeSSS->setText("<span style='color:#27AE60; font-size:16px;'>✓ " + QString::number(chemins.size()) + " fichier(s) valide(s)</span>");
            erreurSSS->setText(""); mettreAJourOngletFichier(0, "accepte");
            creerChampsPrefixes(chemins, layoutPrefixesSSS, prefixesParFichierSSS, "Ex: SYST_");
        } else {
            cheminsSSS.clear();
            badgeSSS->setText("<span style='color:#E74C3C; font-size:16px;'>✗ Format invalide</span>");
            erreurSSS->setText("⚠ Acceptés : .docx  .doc  .csv  .xls  .xlsx"); mettreAJourOngletFichier(0, "erreur");
        }
        verifierFichiersSelectionnes();
    }
}

// ─── parcourirSRS ─────────────────────────────────────────────────────────────
/**
 * @brief Sélection multiple de fichiers SRS.
 */
void MainWindow::parcourirSRS()
{
    QStringList chemins = QFileDialog::getOpenFileNames(
        this, "Sélectionner les fichiers SRS", "",
        "Fichiers supportés (*.docx *.doc *.csv *.xls *.xlsx *.txt);;Tous (*)");
    if (!chemins.isEmpty()) {
        cheminsSRS = chemins; champSRS->setText(chemins.join(" | ")); listeSRS->clear();
        bool tousValides = true;
        for (const QString &c : chemins) {
            QFileInfo info(c);
            if (formatAccepte(c)) { QListWidgetItem *it = new QListWidgetItem("📄 " + info.fileName()); it->setForeground(QColor("#333333")); listeSRS->addItem(it); }
            else { QListWidgetItem *it = new QListWidgetItem("⚠ " + info.fileName()); it->setForeground(QColor("#E74C3C")); listeSRS->addItem(it); tousValides = false; }
        }
        if (tousValides) {
            badgeSRS->setText("<span style='color:#27AE60; font-size:16px;'>✓ " + QString::number(chemins.size()) + " fichier(s) valide(s)</span>");
            erreurSRS->setText(""); mettreAJourOngletFichier(1, "accepte");
            creerChampsPrefixes(chemins, layoutPrefixesSRS, prefixesParFichierSRS, "Ex: EXIGENCE_");
        } else {
            cheminsSRS.clear();
            badgeSRS->setText("<span style='color:#E74C3C; font-size:16px;'>✗ Format invalide</span>");
            erreurSRS->setText("⚠ Acceptés : .docx  .doc  .csv  .xls  .xlsx"); mettreAJourOngletFichier(1, "erreur");
        }
        verifierFichiersSelectionnes();
    }
}

// ─── parcourirSDD ─────────────────────────────────────────────────────────────
/**
 * @brief Sélection multiple de fichiers SDD (optionnel).
 */
void MainWindow::parcourirSDD()
{
    QStringList chemins = QFileDialog::getOpenFileNames(
        this, "Sélectionner les fichiers SDD", "",
        "Fichiers supportés (*.docx *.doc *.csv *.xls *.xlsx *.txt);;Tous (*)");
    if (!chemins.isEmpty()) {
        cheminsSDD = chemins; champSDD->setText(chemins.join(" | ")); listeSDD->clear();
        bool tousValides = true;
        for (const QString &c : chemins) {
            QFileInfo info(c);
            if (formatAccepte(c)) { QListWidgetItem *it = new QListWidgetItem("📄 " + info.fileName()); it->setForeground(QColor("#333333")); listeSDD->addItem(it); }
            else { QListWidgetItem *it = new QListWidgetItem("⚠ " + info.fileName()); it->setForeground(QColor("#E74C3C")); listeSDD->addItem(it); tousValides = false; }
        }
        if (tousValides) {
            badgeSDD->setText("<span style='color:#27AE60; font-size:16px;'>✓ " + QString::number(chemins.size()) + " fichier(s) valide(s)</span>");
            erreurSDD->setText(""); mettreAJourOngletFichier(2, "accepte");
            creerChampsPrefixes(chemins, layoutPrefixesSDD, prefixesParFichierSDD, "Ex: CONCEPTION_");
        } else {
            cheminsSDD.clear();
            badgeSDD->setText("<span style='color:#E74C3C; font-size:16px;'>✗ Format invalide</span>");
            erreurSDD->setText("⚠ Acceptés : .docx  .doc  .csv  .xls  .xlsx"); mettreAJourOngletFichier(2, "erreur");
        }
    }
}

// ─── verifierFichiersSelectionnes ────────────────────────────────────────────
/**
 * @brief Active le bouton Lancer si SSS et SRS sont valides.
 */
void MainWindow::verifierFichiersSelectionnes()
{
    bool pret = !cheminsSSS.isEmpty() && !cheminsSRS.isEmpty();
    btnLancer->setEnabled(pret);
    if (pret) {
        labelStatut->setText("✅  Prêt à analyser !");
        labelStatut->setStyleSheet("color: #27AE60; font-size: 10px; padding: 6px; background-color: #F0FFF4; border-radius: 6px; border: 1px solid #27AE60;");
    } else {
        labelStatut->setText("⏳  En attente de fichiers SSS et SRS...");
        labelStatut->setStyleSheet("color: #888888; font-size: 10px; padding: 6px; background-color: #FFFFFF; border-radius: 6px; border: 1px solid #DDDDDD;");
    }
}

// ─── reinitialiser ────────────────────────────────────────────────────────────
/**
 * @brief Remet l'interface à l'état initial.
 */
void MainWindow::reinitialiser()
{
    champSSS->clear(); champSRS->clear(); champSDD->clear();
    badgeSSS->clear(); badgeSRS->clear(); badgeSDD->clear();
    erreurSSS->clear(); erreurSRS->clear(); erreurSDD->clear();

    listeSSS->clear(); listeSRS->clear(); listeSDD->clear();
    ajouterItemVide(listeSSS); ajouterItemVide(listeSRS); ajouterItemVide(listeSDD);

    cheminsSSS.clear(); cheminsSRS.clear(); cheminsSDD.clear();
    mettreAJourOngletFichier(0, "attente"); mettreAJourOngletFichier(1, "attente"); mettreAJourOngletFichier(2, "attente");

    prefixesParFichierSSS.clear(); prefixesParFichierSRS.clear(); prefixesParFichierSDD.clear();
    while (QLayoutItem *item = layoutPrefixesSSS->takeAt(0)) { if (item->widget()) item->widget()->deleteLater(); delete item; }
    while (QLayoutItem *item = layoutPrefixesSRS->takeAt(0)) { if (item->widget()) item->widget()->deleteLater(); delete item; }
    while (QLayoutItem *item = layoutPrefixesSDD->takeAt(0)) { if (item->widget()) item->widget()->deleteLater(); delete item; }

    champRechercheId->clear(); btnLancer->setEnabled(false); terminal->clear(); barreProgression->setValue(0);
    listeExigences->clear(); toutesExigences.clear(); labelNbExigences->setText("0 exigence(s) détectée(s)");
    labelStatut->setText("⏳  En attente de fichiers...");
    labelStatut->setStyleSheet("color: #888888; font-size: 10px; padding: 6px; background-color: #FFFFFF; border-radius: 6px; border: 1px solid #DDDDDD;");
}

// ─── logTerminal ──────────────────────────────────────────────────────────────
/**
 * @brief Ajoute un message coloré dans le terminal d'analyse.
 * @param message Le texte à afficher.
 * @param couleur Couleur HTML.
 */
void MainWindow::logTerminal(const QString &message, const QString &couleur)
{
    terminal->append(QString("<span style='color:%1;'>%2</span>").arg(couleur, message));
}

// ─── afficherApercuExigences ──────────────────────────────────────────────────
/**
 * @brief Affiche les exigences extraites dans l'aperçu à droite.
 * SSS en bleu, SRS en orange, SDD en violet.
 */
void MainWindow::afficherApercuExigences(const std::vector<ExigenceExtraite> &sss,
                                         const std::vector<ExigenceExtraite> &srs,
                                         const std::vector<ExigenceExtraite> &sdd)
{
    toutesExigences.clear(); listeExigences->clear();
    for (const auto &e : sss) toutesExigences.append("[SSS] " + QString::fromStdString(e.identifiant));
    for (const auto &e : srs) toutesExigences.append("[SRS] " + QString::fromStdString(e.identifiant));
    for (const auto &e : sdd) toutesExigences.append("[SDD] " + QString::fromStdString(e.identifiant));
    for (const QString &item : toutesExigences) {
        QListWidgetItem *it = new QListWidgetItem(item);
        if (item.startsWith("[SSS]"))      it->setForeground(QColor("#2E86C1"));
        else if (item.startsWith("[SRS]")) it->setForeground(QColor("#E67E22"));
        else                               it->setForeground(QColor("#8E44AD"));
        listeExigences->addItem(it);
    }
    int total = (int)(sss.size() + srs.size() + sdd.size());
    labelNbExigences->setText(QString::number(total) + " exigence(s) détectée(s)");
}

// ─── filtrerApercu ────────────────────────────────────────────────────────────
/**
 * @brief Filtre l'aperçu des exigences par ID en temps réel.
 * @param texte Texte saisi dans le champ de recherche.
 */
void MainWindow::filtrerApercu(const QString &texte)
{
    listeExigences->clear();
    for (const QString &item : toutesExigences) {
        if (texte.isEmpty() || item.contains(texte, Qt::CaseInsensitive)) {
            QListWidgetItem *it = new QListWidgetItem(item);
            if (item.startsWith("[SSS]"))      it->setForeground(QColor("#2E86C1"));
            else if (item.startsWith("[SRS]")) it->setForeground(QColor("#E67E22"));
            else                               it->setForeground(QColor("#8E44AD"));
            listeExigences->addItem(it);
        }
    }
    int nb = listeExigences->count(); int total = toutesExigences.size();
    if (texte.isEmpty()) labelNbExigences->setText(QString::number(total) + " exigence(s) détectée(s)");
    else labelNbExigences->setText(QString::number(nb) + " / " + QString::number(total) + " exigence(s)");
}

// ─── lancerAnalyse ────────────────────────────────────────────────────────────
/**
 * @brief Lance le pipeline complet d'analyse de traçabilité.
 *
 * 1. Vérification des préfixes → 2. Construction regex → 3. Extraction M1
 * → 4. Conversion → 5. Thread M2 → 6. Affichage résultats
 */
void MainWindow::lancerAnalyse()
{
    try {
        terminal->clear(); barreProgression->setValue(0); btnLancer->setEnabled(false);
        logTerminal("▶ Lancement de l'analyse...", "#4A90D9");

        // ── Vérifier que tous les fichiers SSS et SRS ont un préfixe ──
        for (const QString &chemin : cheminsSSS) {
            if (!prefixesParFichierSSS.contains(chemin) || prefixesParFichierSSS[chemin]->text().trimmed().isEmpty()) {
                logTerminal("❌ Préfixe manquant pour SSS : " + QFileInfo(chemin).fileName(), "#E74C3C");
                btnLancer->setEnabled(true); return;
            }
        }
        for (const QString &chemin : cheminsSRS) {
            if (!prefixesParFichierSRS.contains(chemin) || prefixesParFichierSRS[chemin]->text().trimmed().isEmpty()) {
                logTerminal("❌ Préfixe manquant pour SRS : " + QFileInfo(chemin).fileName(), "#E74C3C");
                btnLancer->setEnabled(true); return;
            }
        }

        // ── Helpers ──
        auto splitPrefixes = [](const QString &raw) -> QStringList {
            QStringList result;
            for (const QString &p : raw.split(",", Qt::SkipEmptyParts)) { QString t = p.trimmed(); if (!t.isEmpty()) result.append(t); }
            return result;
        };
        auto construireMotif = [](const QStringList &prefs) -> std::string {
            if (prefs.isEmpty()) return "";
            if (prefs.size() == 1) return prefs[0].toStdString() + "[A-Z0-9_]+";
            std::string motif = "(";
            for (int k = 0; k < prefs.size(); k++) { if (k > 0) motif += "|"; motif += prefs[k].toStdString(); }
            motif += ")[A-Z0-9_]+"; return motif;
        };

        // ── Motif pour extraction SRS (SSS + SRS uniquement) ──
        QStringList prefixesPourSRS;
        for (const QString &c : cheminsSSS) prefixesPourSRS += splitPrefixes(prefixesParFichierSSS[c]->text());
        for (const QString &c : cheminsSRS)  prefixesPourSRS += splitPrefixes(prefixesParFichierSRS[c]->text());
        prefixesPourSRS.removeDuplicates();
        std::string motifSRS = construireMotif(prefixesPourSRS);

        // ── Motif pour extraction SDD (SRS + SDD) ──
        // ── Motif pour extraction SDD (SDD uniquement pour les IDs) ──
        QStringList prefixesPourSDD;
        for (const QString &c : cheminsSDD) {
            if (prefixesParFichierSDD.contains(c) && !prefixesParFichierSDD[c]->text().trimmed().isEmpty())
                prefixesPourSDD += splitPrefixes(prefixesParFichierSDD[c]->text());
        }
        prefixesPourSDD.removeDuplicates();
        std::string motifSDD = construireMotif(prefixesPourSDD);

        logTerminal("⚙ Motif SRS : " + QString::fromStdString(motifSRS), "#AED6F1");
        logTerminal("⚙ Motif SDD : " + QString::fromStdString(motifSDD), "#AED6F1");

        std::vector<ExigenceExtraite> extraitesSSS, extraitesSRS, extraitesSDD;

        // ── Extraction SSS ──
        for (const QString &chemin : cheminsSSS) {
            QStringList prefs = splitPrefixes(prefixesParFichierSSS[chemin]->text());
            logTerminal("📂 Lecture SSS : " + QFileInfo(chemin).fileName() + "  [préfixe(s): " + prefs.join(", ") + "]", "#888888");
            ExtracteurFichier extracteur(chemin.toStdString(), construireMotif(prefs));
            auto res = extracteur.extraire(); extraitesSSS.insert(extraitesSSS.end(), res.begin(), res.end());
            logTerminal("✅ " + QString::number(res.size()) + " exigences SSS détectées", "#27AE60");
        }
        barreProgression->setValue(20);

        // ── Extraction SRS ──
        for (const QString &chemin : cheminsSRS) {
            QStringList prefs = splitPrefixes(prefixesParFichierSRS[chemin]->text());
            logTerminal("📂 Lecture SRS : " + QFileInfo(chemin).fileName() + "  [préfixe(s): " + prefs.join(", ") + "]", "#888888");
            ExtracteurFichier extracteur(chemin.toStdString(), motifSRS, "[A-Z][A-Z0-9]*_[A-Z][A-Z0-9]*_[A-Z0-9_]+");
            auto res = extracteur.extraire(); extraitesSRS.insert(extraitesSRS.end(), res.begin(), res.end());
            logTerminal("✅ " + QString::number(res.size()) + " exigences SRS détectées", "#27AE60");
        }
        // ── Filtrer extraitesSRS : garder uniquement les vrais IDs SRS ──
        QStringList prefsSRS;
        for (const QString &c : cheminsSRS) {
            if (prefixesParFichierSRS.contains(c) && !prefixesParFichierSRS[c]->text().trimmed().isEmpty())
                prefsSRS += splitPrefixes(prefixesParFichierSRS[c]->text());
        }
        prefsSRS.removeDuplicates();

        std::vector<ExigenceExtraite> extraitesSRS_filtrees;
        for (const auto& ex : extraitesSRS) {
            for (const QString& pref : prefsSRS) {
                if (ex.identifiant.find(pref.toStdString()) == 0) {
                    extraitesSRS_filtrees.push_back(ex);
                    break;
                }
            }
        }
        extraitesSRS = extraitesSRS_filtrees;

        barreProgression->setValue(40);

        // ── Extraction SDD ──
        for (const QString &chemin : cheminsSDD) {
            if (!prefixesParFichierSDD.contains(chemin) || prefixesParFichierSDD[chemin]->text().trimmed().isEmpty()) {
                logTerminal("⚠ Pas de préfixe pour SDD : " + QFileInfo(chemin).fileName() + " — ignoré", "#E67E22"); continue;
            }
            QStringList prefs = splitPrefixes(prefixesParFichierSDD[chemin]->text());
            logTerminal("📂 Lecture SDD : " + QFileInfo(chemin).fileName() + "  [préfixe(s): " + prefs.join(", ") + "]", "#888888");
            ExtracteurFichier extracteur(chemin.toStdString(), motifSDD, "[A-Z][A-Z0-9]*_[A-Z][A-Z0-9]*_[A-Z0-9_]+");
            auto res = extracteur.extraire(); extraitesSDD.insert(extraitesSDD.end(), res.begin(), res.end());
            logTerminal("✅ " + QString::number(res.size()) + " exigences SDD détectées", "#27AE60");
        }
        // ── Filtrer extraitesSDD : garder uniquement les vrais IDs SDD ──
        QStringList prefsSDD;
        for (const QString &c : cheminsSDD) {
            if (prefixesParFichierSDD.contains(c) && !prefixesParFichierSDD[c]->text().trimmed().isEmpty())
                prefsSDD += splitPrefixes(prefixesParFichierSDD[c]->text());
        }
        prefsSDD.removeDuplicates();

        std::vector<ExigenceExtraite> extraitesSDD_filtrees;
        for (const auto& ex : extraitesSDD) {
            // Cas SDD 1 colonne : ID auto-généré commence par "SDD_" → accepter
            if (ex.identifiant.find("SDD_") == 0) {
                extraitesSDD_filtrees.push_back(ex);
                continue;
            }
            // Cas normal : vérifier le préfixe
            for (const QString& pref : prefsSDD) {
                if (ex.identifiant.find(pref.toStdString()) == 0) {
                    extraitesSDD_filtrees.push_back(ex);
                    break;
                }
            }
        }
        extraitesSDD = extraitesSDD_filtrees;

        barreProgression->setValue(55);


        logTerminal("📊 Total extrait : " + QString::number(extraitesSSS.size()) + " SSS | "
                        + QString::number(extraitesSRS.size()) + " SRS | " + QString::number(extraitesSDD.size()) + " SDD", "#4A90D9");
        afficherApercuExigences(extraitesSSS, extraitesSRS, extraitesSDD);

        // ── Conversion ──
        QStringList listePrefsSSS, listePrefsSRS, listePrefsSDD;
        for (const QString &c : cheminsSSS) listePrefsSSS += splitPrefixes(prefixesParFichierSSS[c]->text());
        for (const QString &c : cheminsSRS)  listePrefsSRS += splitPrefixes(prefixesParFichierSRS[c]->text());
        for (const QString &c : cheminsSDD) {
            if (prefixesParFichierSDD.contains(c) && !prefixesParFichierSDD[c]->text().trimmed().isEmpty())
                listePrefsSDD += splitPrefixes(prefixesParFichierSDD[c]->text());
        }
        listePrefsSSS.removeDuplicates(); listePrefsSRS.removeDuplicates(); listePrefsSDD.removeDuplicates();
        std::string prefSSS = listePrefsSSS.join(",").toStdString();
        std::string prefSRS = listePrefsSRS.join(",").toStdString();
        std::string prefSDD = listePrefsSDD.join(",").toStdString();

        logTerminal("⚙ Conversion des exigences...", "#E67E22");

        // SSS
        ConvertisseurExigences convSSS(motifSRS, prefSSS, prefSRS, prefSDD);
        convSSS.convertir(extraitesSSS);
        auto sss = convSSS.getSSS();

        // SRS
        ConvertisseurExigences convSRS(motifSRS, prefSSS, prefSRS, prefSDD);
        convSRS.convertir(extraitesSRS);
        auto srs = convSRS.getSRS();

        // SDD : forcer le type SDD directement
        std::vector<ExigenceSDD> sdd;
        for (const auto& ex : extraitesSDD) {
            ExigenceSDD obj(ex.identifiant, ex.description);
            obj.setReferencedSRS(ex.tracabilite);
            obj.setDeveloppe(ex.developpe);
            obj.setFichierSource(ex.fichierSource);
            sdd.push_back(obj);
        }

        barreProgression->setValue(65);

        // ── Thread M2 ──
        logTerminal("⚙ Analyse de traçabilité en cours (thread)...", "#E67E22");
        moteurAnalyse = new MoteurTracabilite();
        moteurAnalyse->setDonnees(sss, srs, sdd);
        threadManager = new ThreadManager(*moteurAnalyse);
        threadManager->lancerAnalyse();

        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this, timer, srs, sdd]() {
            if (threadManager->estTerminee()) {
                timer->stop(); timer->deleteLater();
                barreProgression->setValue(100);
                RapportTracabilite rapport = moteurAnalyse->getRapport();

                logTerminal("✅ Analyse terminée !", "#27AE60");
                logTerminal("📊 SSS→SRS : " + QString::number(rapport.tauxSSS_SRS * 100, 'f', 1) + "%", "#4A90D9");
                logTerminal("📊 SRS→SDD : " + QString::number(rapport.tauxSRS_SDD * 100, 'f', 1) + "%", "#4A90D9");
                logTerminal("📊 Global  : " + QString::number(rapport.tauxGlobal * 100, 'f', 1) + "%", "#4A90D9");
                if (!rapport.liensBrises.empty())
                    logTerminal("⚠ " + QString::number(rapport.liensBrises.size()) + " lien(s) brisé(s) détecté(s)", "#E67E22");

                ongletRapport->afficherRapportComplet(rapport, srs, sdd, moteurAnalyse->getOrphelines());
                ongletGraphe->construireGraphe(rapport, srs, sdd, moteurAnalyse->getOrphelines());

                QStringList nomsFichiers;
                for (const auto &s : rapport.exigencesSSS) { QString nom = QFileInfo(QString::fromStdString(s.getFichierSource())).fileName(); if (!nomsFichiers.contains(nom)) nomsFichiers.append(nom); }
                for (const auto &s : srs) { QString nom = QFileInfo(QString::fromStdString(s.getFichierSource())).fileName(); if (!nomsFichiers.contains(nom)) nomsFichiers.append(nom); }
                for (const auto &d : sdd) { QString nom = QFileInfo(QString::fromStdString(d.getFichierSource())).fileName(); if (!nomsFichiers.contains(nom)) nomsFichiers.append(nom); }
                ongletGraphe->setListeNomsFichiers(nomsFichiers);
                gestionnaireFiltres->reinitialiser();
                tabWidget->setCurrentWidget(ongletRapportConteneur);

                labelStatut->setText("✅  Analyse terminée !");
                labelStatut->setStyleSheet("color: #27AE60; font-size: 10px; padding: 6px; background-color: #F0FFF4; border-radius: 6px; border: 1px solid #27AE60;");
                btnLancer->setEnabled(true);
                delete threadManager; delete moteurAnalyse; threadManager = nullptr; moteurAnalyse = nullptr;
            } else {
                int val = barreProgression->value(); if (val < 95) barreProgression->setValue(val + 1);
            }
        });
        timer->start(100);

    } catch (const FileNotFoundException &e) {
        logTerminal("❌ Fichier introuvable : " + QString::fromStdString(e.what()), "#E74C3C");
        QMessageBox::critical(this, "Erreur", QString::fromStdString(e.what()));
        btnLancer->setEnabled(true); barreProgression->setValue(0);
        labelStatut->setText("❌  Fichier introuvable !");
        labelStatut->setStyleSheet("color:#E74C3C; font-size:10px; padding:6px; background-color:#FFF5F5; border-radius:6px; border:1px solid #E74C3C;");
    } catch (const FileFormatException &e) {
        logTerminal("❌ Format non supporté : " + QString::fromStdString(e.what()), "#E74C3C");
        QMessageBox::critical(this, "Erreur", QString::fromStdString(e.what()));
        btnLancer->setEnabled(true); barreProgression->setValue(0);
        labelStatut->setText("❌  Format non supporté !");
        labelStatut->setStyleSheet("color:#E74C3C; font-size:10px; padding:6px; background-color:#FFF5F5; border-radius:6px; border:1px solid #E74C3C;");
    } catch (const std::exception &e) {
        logTerminal("❌ Erreur : " + QString::fromStdString(e.what()), "#E74C3C");
        QMessageBox::critical(this, "Erreur", QString::fromStdString(e.what()));
        btnLancer->setEnabled(true); barreProgression->setValue(0);
        labelStatut->setText("❌  Erreur lors de l'analyse !");
        labelStatut->setStyleSheet("color:#E74C3C; font-size:10px; padding:6px; background-color:#FFF5F5; border-radius:6px; border:1px solid #E74C3C;");
    }
}
