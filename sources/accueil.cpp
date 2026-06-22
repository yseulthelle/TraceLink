/**
 * @file accueil.cpp
 * @brief Fenêtre de démarrage TraceLink.
 * @date 26/04/2026
 * @version 2.1
 */

#include "accueil.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QGraphicsDropShadowEffect>

/**
 * @brief Constructeur de la fenêtre de démarrage.
 * @param parent Widget parent.
 */
Accueil::Accueil(QWidget *parent)
    : QWidget(parent), progression(0)
{
    setWindowTitle("TraceLink");
    setFixedSize(560, 300);
    setWindowFlags(Qt::FramelessWindowHint);
    setupUI();
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Accueil::avancerChargement);
    timer->start(25);
}

/**
 * @brief Initialise l'interface de la fenêtre de démarrage.
 * Thème clair avec titre en police technique Courier New.
 */
void Accueil::setupUI()
{
    setStyleSheet(R"(
        Accueil {
            background-color: #EBF5FB;
            border-radius: 20px;
            border: 1px solid #AED6F1;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(50, 40, 50, 40);
    layout->setSpacing(0);

    // Bandeau gradient en haut
    QWidget *bandeau = new QWidget();
    bandeau->setFixedHeight(8);
    bandeau->setStyleSheet(R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
            stop:0 #1A5276, stop:0.5 #2E86C1, stop:1 #AED6F1);
        border-radius: 4px;
    )");

    // Nom du logiciel
    QLabel *labelNom = new QLabel("TraceLink");
    labelNom->setAlignment(Qt::AlignCenter);
    labelNom->setStyleSheet(R"(
        font-size: 58px;
        font-weight: 900;
        color: #1A5276;
        letter-spacing: 8px;
        font-family: 'Courier New', 'Consolas', monospace;
    )");

    // ombre portée sur le titre
    QGraphicsDropShadowEffect *ombre = new QGraphicsDropShadowEffect();
    ombre->setBlurRadius(20);
    ombre->setColor(QColor(46, 134, 193, 80));
    ombre->setOffset(0, 4);
    labelNom->setGraphicsEffect(ombre);

    //  Ligne décorative sous le titre
    QWidget *ligne = new QWidget();
    ligne->setFixedHeight(3);
    ligne->setStyleSheet(R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
            stop:0 transparent, stop:0.3 #2E86C1, stop:0.7 #2E86C1, stop:1 transparent);
        border-radius: 2px;
    )");

    //  Sous-titre
    QLabel *labelSous = new QLabel("Outil de traçabilité automatisé  —  L2G1");
    labelSous->setAlignment(Qt::AlignCenter);
    labelSous->setStyleSheet(R"(
        color: #7FB3D3;
        font-size: 11px;
        font-weight: bold;
        letter-spacing: 2px;
    )");

    //  Barre de chargement
    barreChargement = new QProgressBar();
    barreChargement->setRange(0, 100);
    barreChargement->setValue(0);
    barreChargement->setTextVisible(false);
    barreChargement->setFixedHeight(8);
    barreChargement->setStyleSheet(R"(
        QProgressBar {
            background-color: #D6EAF8;
            border-radius: 4px;
            border: none;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #1A5276, stop:1 #2E86C1);
            border-radius: 4px;
        }
    )");

    //  Label animé de chargement
    labelChargement = new QLabel("Initialisation...");
    labelChargement->setAlignment(Qt::AlignCenter);
    labelChargement->setStyleSheet("color: #AED6F1; font-size: 10px;");

    //  Bouton Démarrer
    btnDemarrer = new QPushButton("▶   Démarrer");
    btnDemarrer->setEnabled(false);
    btnDemarrer->setFixedHeight(44);
    btnDemarrer->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #1A5276, stop:1 #2E86C1);
            color: white;
            border-radius: 12px;
            font-size: 14px;
            font-weight: bold;
            border: none;
            letter-spacing: 2px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #154360, stop:1 #1A5276);
        }
        QPushButton:disabled {
            background-color: #D6EAF8;
            color: #AED6F1;
            border: none;
        }
    )");

    //  Assemblage
    layout->addWidget(bandeau);
    layout->addSpacing(20);
    layout->addWidget(labelNom);
    layout->addSpacing(6);
    layout->addWidget(ligne);
    layout->addSpacing(8);
    layout->addWidget(labelSous);
    layout->addSpacing(40);
    layout->addWidget(barreChargement);
    layout->addSpacing(4);
    layout->addWidget(labelChargement);
    layout->addSpacing(10);
    layout->addWidget(btnDemarrer);

    connect(btnDemarrer, &QPushButton::clicked, this, &Accueil::demarrerClique);
}

/**
 * @brief Avance la barre de chargement automatiquement.
 * Active le bouton Démarrer quand le chargement est terminé
 */
void Accueil::avancerChargement()
{
    progression++;
    barreChargement->setValue(progression);

    if (progression < 30)
        labelChargement->setText("Initialisation...");
    else if (progression < 60)
        labelChargement->setText("Chargement des modules...");
    else if (progression < 90)
        labelChargement->setText("Préparation de l'interface...");
    else
        labelChargement->setText("Presque prêt...");

    if (progression >= 100) {
        timer->stop();
        labelChargement->setText("");
        btnDemarrer->setEnabled(true);
    }
}