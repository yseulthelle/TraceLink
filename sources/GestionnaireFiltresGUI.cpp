/**
 * @file GestionnaireFiltresGUI.cpp
 * @brief Implémentation de la classe GestionnaireFiltresGUI.
 * @date 02/04/2026
 * @version 1.1 — modeGraphe : cache Statut + État, retire les curseurs SSS
 */

#include "GestionnaireFiltresGUI.h"
#include <QHBoxLayout>
#include <QFrame>
#include <QCompleter>
#include <QStringListModel>
#include <QGraphicsDropShadowEffect>

GestionnaireFiltresGUI::GestionnaireFiltresGUI(QWidget *parent, bool modeGraphe)
    : QWidget(parent), modeGraphe(modeGraphe)
{
    initialiserUI();
}

QLabel* GestionnaireFiltresGUI::creerLabel(const QString& texte)
{
    QLabel *lbl = new QLabel(texte, this);
    lbl->setStyleSheet(R"(
        color: #7F8C9A;
        font-size: 10px;
        font-weight: bold;
        letter-spacing: 1px;
        margin-top: 8px;
        text-transform: uppercase;
    )");
    return lbl;
}

void GestionnaireFiltresGUI::initialiserUI()
{
    // ── Style global du panneau ───────────────────────────────────────────────
    setStyleSheet(R"(
        GestionnaireFiltresGUI {
            background-color: #F8FAFC;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(4);

    // ── Titre ─────────────────────────────────────────────────────────────────
    QHBoxLayout *rowTitre = new QHBoxLayout();
    QLabel *icone = new QLabel("⚙", this);
    icone->setStyleSheet("font-size: 14px;");
    QLabel *titre = new QLabel("Filtres", this);
    titre->setStyleSheet(R"(
        color: #1A3A5C;
        font-size: 14px;
        font-weight: bold;
        letter-spacing: 0.5px;
    )");
    rowTitre->addWidget(icone);
    rowTitre->addWidget(titre);
    rowTitre->addStretch();
    layout->addLayout(rowTitre);

    // ── Séparateur décoratif ──────────────────────────────────────────────────
    QFrame *sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2E86C1, stop:1 transparent); border: none; max-height: 2px; margin-bottom: 8px;");
    layout->addWidget(sep1);
    layout->addSpacing(4);

    // ── Style commun des combos ───────────────────────────────────────────────
    QString styleCombo = R"(
        QComboBox {
            background-color: white;
            color: #2C3E50;
            border: 1.5px solid #D5E8F3;
            border-radius: 8px;
            padding: 7px 12px;
            font-size: 11px;
            font-weight: 500;
        }
        QComboBox:hover {
            border: 1.5px solid #2E86C1;
        }
        QComboBox:focus {
            border: 1.5px solid #2E86C1;
            background-color: #F0F8FF;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 5px solid #2E86C1;
            margin-right: 8px;
        }
        QComboBox QAbstractItemView {
            background: white;
            color: #2C3E50;
            border: 1px solid #D5E8F3;
            border-radius: 6px;
            selection-background-color: #EBF5FB;
            selection-color: #1A5276;
            padding: 4px;
        }
    )";

    // ── Statut ────────────────────────────────────────────────────────────────
    QLabel *lblStatut = creerLabel("Statut");
    layout->addWidget(lblStatut);
    comboStatut = new QComboBox(this);
    comboStatut->addItem("🔵  Tous les statuts",  -1);
    comboStatut->addItem("🟢  Couvert",            Couverte);
    comboStatut->addItem("🔴  Non couvert",        NonCouverte);
    comboStatut->addItem("🟠  Lien brisé",         98);
    comboStatut->addItem("🟣  Orpheline",          99);
    comboStatut->setStyleSheet(styleCombo);
    layout->addWidget(comboStatut);
    layout->addSpacing(4);

    // ── État développé ────────────────────────────────────────────────────────
    QLabel *lblEtat = creerLabel("État");
    layout->addWidget(lblEtat);
    comboEtat = new QComboBox(this);
    comboEtat->addItem("🔵  Tous les états",   0);
    comboEtat->addItem("✅  Développé",         1);
    comboEtat->addItem("⏳  Non développé",     2);
    comboEtat->addItem("🔄  En cours",          3);
    comboEtat->setStyleSheet(styleCombo);
    layout->addWidget(comboEtat);
    layout->addSpacing(4);

    // ── Cacher Statut + État en mode graphe (inutiles pour le graphe) ─────────
    if (modeGraphe) {
        lblStatut->setVisible(false);
        comboStatut->setVisible(false);
        lblEtat->setVisible(false);
        comboEtat->setVisible(false);
    }

    // ── Recherche par mots-clés ───────────────────────────────────────────────
    layout->addWidget(creerLabel("Recherche"));

    QFrame *conteneurRecherche = new QFrame(this);
    conteneurRecherche->setStyleSheet(R"(
        QFrame {
            background-color: white;
            border: 1.5px solid #D5E8F3;
            border-radius: 8px;
        }
        QFrame:focus-within {
            border: 1.5px solid #2E86C1;
            background-color: #F0F8FF;
        }
    )");
    QHBoxLayout *rowRecherche = new QHBoxLayout(conteneurRecherche);
    rowRecherche->setContentsMargins(10, 0, 10, 0);
    rowRecherche->setSpacing(6);

    QLabel *iconeRecherche = new QLabel("🔍", conteneurRecherche);
    iconeRecherche->setStyleSheet("font-size: 12px; background: transparent; border: none;");

    champIdentifiant = new QLineEdit(conteneurRecherche);
    champIdentifiant->setPlaceholderText("Mot-clé, ID partiel...");
    champIdentifiant->setStyleSheet(R"(
        QLineEdit {
            background-color: transparent;
            color: #2C3E50;
            border: none;
            font-size: 11px;
            padding: 7px 0px;
        }
    )");

    completer = new QCompleter(listeIds, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    champIdentifiant->setCompleter(completer);

    rowRecherche->addWidget(iconeRecherche);
    rowRecherche->addWidget(champIdentifiant, 1);
    layout->addWidget(conteneurRecherche);
    layout->addSpacing(12);

    // ── Bouton Appliquer ──────────────────────────────────────────────────────
    btnAppliquer = new QPushButton("Appliquer", this);
    btnAppliquer->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #2E86C1, stop:1 #1A5276);
            color: white;
            border-radius: 8px;
            padding: 9px;
            font-size: 12px;
            font-weight: bold;
            letter-spacing: 0.5px;
            border: none;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #3498DB, stop:1 #2E86C1);
        }
        QPushButton:pressed {
            background: #154360;
        }
    )");
    connect(btnAppliquer, &QPushButton::clicked, this, &GestionnaireFiltresGUI::appliquerFiltres);
    layout->addWidget(btnAppliquer);

    // ── Séparateur ────────────────────────────────────────────────────────────
    layout->addSpacing(12);
    QFrame *sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("background-color: #E8F1F8; border: none; max-height: 1px;");
    layout->addWidget(sep2);
    layout->addSpacing(8);

    // ── Légende ───────────────────────────────────────────────────────────────
    QLabel *titreLeg = new QLabel("LÉGENDE", this);
    titreLeg->setStyleSheet("color: #AAB7C4; font-size: 9px; font-weight: bold; letter-spacing: 2px;");
    layout->addWidget(titreLeg);
    layout->addSpacing(6);

    auto ajouterLegende = [&](const QString& texte, const QString& couleur, const QString& desc) {
        QHBoxLayout *row = new QHBoxLayout();
        row->setSpacing(8);

        QLabel *carre = new QLabel(this);
        carre->setFixedSize(10, 10);
        carre->setStyleSheet(QString(R"(
            background-color: %1;
            border-radius: 2px;
        )").arg(couleur));

        QLabel *lbl = new QLabel(texte, this);
        lbl->setStyleSheet("color: #4A5568; font-size: 11px; font-weight: 500;");

        Q_UNUSED(desc);
        row->addWidget(carre);
        row->addWidget(lbl);
        row->addStretch();
        layout->addLayout(row);
        layout->addSpacing(2);
    };

    ajouterLegende("Couvert",      "#27AE60", "Relié à au moins une exigence inférieure");
    ajouterLegende("Non couvert",  "#E74C3C", "Aucun lien vers le niveau inférieur");
    ajouterLegende("Lien brisé",   "#E67E22", "Référence vers un ID inexistant");
    ajouterLegende("Orpheline",    "#7D3C98", "Sans référence vers le niveau supérieur");

    layout->addStretch();

    // ── Bouton Réinitialiser ──────────────────────────────────────────────────
    btnReinitialiser = new QPushButton("↺  Réinitialiser", this);
    btnReinitialiser->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            color: #7F8C9A;
            border: 1.5px solid #D5E8F3;
            border-radius: 8px;
            padding: 7px;
            font-size: 11px;
        }
        QPushButton:hover {
            background-color: #EBF5FB;
            color: #2E86C1;
            border-color: #2E86C1;
        }
    )");
    connect(btnReinitialiser, &QPushButton::clicked, this, &GestionnaireFiltresGUI::reinitialiser);
    layout->addWidget(btnReinitialiser);

    setLayout(layout);
}

FiltreGUI GestionnaireFiltresGUI::getFiltreActuel() const
{
    return filtreCourant;
}

void GestionnaireFiltresGUI::appliquerFiltres()
{
    int statut = comboStatut->currentData().toInt();
    filtreCourant.filtreOrpheline  = (statut == 99);
    filtreCourant.filtreLienBrise  = (statut == 98);
    if (statut == 99 || statut == 98)
        filtreCourant.filtreStatut = NonAnalyse; // pas de filtre statut dans ces cas
    else
        filtreCourant.filtreStatut = static_cast<Statut>(statut);
    filtreCourant.filtreIdentifiant = champIdentifiant->text();
    int etat = comboEtat->currentData().toInt();
    filtreCourant.filtreDevEtat    = (etat != 0);
    filtreCourant.filtreDevValeur  = (etat == 1);
    filtreCourant.filtreEnCours    = (etat == 3);
    emit filtreModifie(filtreCourant);
}

void GestionnaireFiltresGUI::setFiltreStatut(int index)
{
    int val = comboStatut->itemData(index).toInt();
    filtreCourant.filtreStatut = static_cast<Statut>(val);
}

void GestionnaireFiltresGUI::setFiltreIdentifiant(const QString& texte)
{
    filtreCourant.filtreIdentifiant = texte;
}

void GestionnaireFiltresGUI::setListeIds(const QStringList& ids)
{
    listeIds = ids;
    completer->setModel(new QStringListModel(listeIds, completer));
}

void GestionnaireFiltresGUI::reinitialiser()
{
    comboStatut->setCurrentIndex(0);
    comboEtat->setCurrentIndex(0);
    champIdentifiant->clear();
    filtreCourant = FiltreGUI();
    emit filtreModifie(filtreCourant);
}
