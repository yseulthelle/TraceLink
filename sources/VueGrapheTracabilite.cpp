/**
 * @file VueGrapheTracabilite.cpp
 * @brief Vue graphe de traçabilité — design amélioré avec 2 vues.
 * @version 7.1 — filtre mot-clé précis + curseurs % SSS
 */
#include "VueGrapheTracabilite.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontMetrics>
#include <QToolTip>
#include <QListWidgetItem>
#include <cmath>
#include <algorithm>
#include <QMap>
#include <QSet>
#include <QLinearGradient>
#include <QPainterPath>

// ─── Couleurs des niveaux ─────────────────────────────────────────────────────
const QColor VueGrapheTracabilite::COULEUR_SSS = QColor("#0D2B45");
const QColor VueGrapheTracabilite::COULEUR_SRS = QColor("#1A6FA8");
const QColor VueGrapheTracabilite::COULEUR_SDD = QColor("#7EC8E3");

// ─── Helpers style boutons ────────────────────────────────────────────────────
void VueGrapheTracabilite::styleBoutonActif(QPushButton* btn, QColor couleur) {
    btn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border-radius: 8px;
            padding: 5px 16px;
            font-size: 11px;
            font-weight: bold;
            border: none;
        }
        QPushButton:hover { background-color: %2; }
    )").arg(couleur.name()).arg(couleur.darker(115).name()));
}

void VueGrapheTracabilite::styleBoutonInactif(QPushButton* btn) {
    btn->setStyleSheet(R"(
        QPushButton {
            background-color: #FFFFFF;
            color: #555555;
            border-radius: 8px;
            padding: 5px 16px;
            font-size: 11px;
            border: 1px solid #CCDDEE;
        }
        QPushButton:hover { background-color: #EBF5FB; color: #1A6FA8; }
    )");
}

// ─── Constructeur ─────────────────────────────────────────────────────────────
VueGrapheTracabilite::VueGrapheTracabilite(QWidget *parent)
    : QWidget(parent),
    filtreRapide(0),
    niveauZoom(1.0),
    noeudSurvole(-1),
    noeudSelectionne(-1),
    lienSurvole(-1),
    vueIndirecte(false),
    filtrePctMin(0),
    filtrePctMax(100),
    filtrePctActif(false)
{
    setMinimumSize(600, 400);
    setStyleSheet("background-color: #F0F7FF;");
    setMouseTracking(true);

    // ── Filtre rapide (caché) ─────────────────────────────────────────────────
    comboFiltreRapide = new QComboBox(this);
    comboFiltreRapide->addItem("👁 Tout afficher");
    comboFiltreRapide->addItem("✗ Non couvertes seulement");
    comboFiltreRapide->addItem("⚠ Liens brisés seulement");
    comboFiltreRapide->setVisible(false);
    connect(comboFiltreRapide, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VueGrapheTracabilite::onFiltreRapideChange);


    // ── Bouton filtres ────────────────────────────────────────────────────────
    boutonFiltres = new QPushButton("⚙  Filtres ▶", this);
    boutonFiltres->resize(110, 32);
    boutonFiltres->move(900, 12);
    boutonFiltres->setStyleSheet(R"(
        QPushButton {
            background-color: #FFFFFF; color: #1A6FA8;
            border-radius: 8px; padding: 5px 12px;
            font-size: 11px; border: 1px solid #CCDDEE;
        }
        QPushButton:hover { background-color: #EBF5FB; }
    )");
    connect(boutonFiltres, &QPushButton::clicked, this, &VueGrapheTracabilite::toggleFiltres);

    // ── Panneau filtres avancés ───────────────────────────────────────────────
    panneauFiltres = new QFrame(this);
    panneauFiltres->setStyleSheet(
        "QFrame { background-color: #FFFFFF; border-left: 2px solid #CCDDEE; }");
    panneauFiltres->setVisible(false);
    QVBoxLayout *layoutFiltres = new QVBoxLayout(panneauFiltres);
    layoutFiltres->setContentsMargins(10, 10, 10, 10);
    layoutFiltres->setSpacing(4);

    // GestionnaireFiltresGUI en mode graphe (cache Statut + État)
    gestionnaireFiltres = new GestionnaireFiltresGUI(panneauFiltres, true);
    layoutFiltres->addWidget(gestionnaireFiltres);

    connect(gestionnaireFiltres, &GestionnaireFiltresGUI::filtreModifie,
            this, &VueGrapheTracabilite::appliquerFiltre);

    // ── Curseurs % SSS — uniquement dans le graphe ────────────────────────────
    QLabel *lblPct = new QLabel("COUVERTURE SSS (%)", panneauFiltres);
    lblPct->setStyleSheet(
        "color: #7F8C9A; font-size: 10px; font-weight: bold; "
        "letter-spacing: 1px; margin-top: 8px;");
    layoutFiltres->addWidget(lblPct);

    QHBoxLayout *rowPctLbl = new QHBoxLayout();
    labelPctMin = new QLabel("Min : 0%", panneauFiltres);
    labelPctMin->setStyleSheet("color: #2C3E50; font-size: 10px;");
    labelPctMax = new QLabel("Max : 100%", panneauFiltres);
    labelPctMax->setStyleSheet("color: #2C3E50; font-size: 10px;");
    rowPctLbl->addWidget(labelPctMin);
    rowPctLbl->addStretch();
    rowPctLbl->addWidget(labelPctMax);
    layoutFiltres->addLayout(rowPctLbl);

    QString styleSlider = R"(
        QSlider::groove:horizontal {
            height: 4px; background: #D5E8F3; border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #2E86C1; border-radius: 7px;
            width: 14px; height: 14px; margin: -5px 0;
        }
        QSlider::sub-page:horizontal { background: #2E86C1; border-radius: 2px; }
    )";

    sliderPctMin = new QSlider(Qt::Horizontal, panneauFiltres);
    sliderPctMin->setRange(0, 100);
    sliderPctMin->setValue(0);
    sliderPctMin->setStyleSheet(styleSlider);
    layoutFiltres->addWidget(sliderPctMin);

    sliderPctMax = new QSlider(Qt::Horizontal, panneauFiltres);
    sliderPctMax->setRange(0, 100);
    sliderPctMax->setValue(100);
    sliderPctMax->setStyleSheet(styleSlider);
    layoutFiltres->addWidget(sliderPctMax);

    // Connexions sliders — lambdas simples sans paramètre inutilisé
    connect(sliderPctMin, &QSlider::valueChanged, this, [this]() {
        if (sliderPctMin->value() > sliderPctMax->value())
            sliderPctMin->setValue(sliderPctMax->value());
        filtrePctMin = sliderPctMin->value();
        labelPctMin->setText(QString("Min : %1%").arg(filtrePctMin));
        filtrePctActif = !(filtrePctMin == 0 && filtrePctMax == 100);
        update();
    });
    connect(sliderPctMax, &QSlider::valueChanged, this, [this]() {
        if (sliderPctMax->value() < sliderPctMin->value())
            sliderPctMax->setValue(sliderPctMin->value());
        filtrePctMax = sliderPctMax->value();
        labelPctMax->setText(QString("Max : %1%").arg(filtrePctMax));
        filtrePctActif = !(filtrePctMin == 0 && filtrePctMax == 100);
        update();
    });

    layoutFiltres->addStretch();

    // ── Panneau info bas ──────────────────────────────────────────────────────
    panneauInfo = new QFrame(this);
    panneauInfo->setStyleSheet(R"(
        QFrame {
            background-color: #0D2B45;
            border-top: 2px solid #1A6FA8;
        }
    )");
    panneauInfo->setVisible(false);

    QHBoxLayout *layoutInfo = new QHBoxLayout(panneauInfo);
    layoutInfo->setContentsMargins(20, 0, 20, 0);
    layoutInfo->setSpacing(24);

    auto makeTauxLabel = [](QWidget* parent, const QString& texte, const QString& couleur) {
        QLabel* l = new QLabel(texte, parent);
        l->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(couleur));
        return l;
    };

    labelTauxSSS_SRS = makeTauxLabel(panneauInfo, "SSS←SRS : —", "#7EC8E3");
    labelTauxSRS_SDD = makeTauxLabel(panneauInfo, "SRS←SDD : —", "#AED6F1");
    labelTauxGlobal  = makeTauxLabel(panneauInfo, "Global : —",  "#FFFFFF");
    labelLiensBrises = makeTauxLabel(panneauInfo, "", "#E74C3C");
    labelLiensBrises->setVisible(false);

    auto makeSep = [](QWidget* parent) {
        QFrame* sep = new QFrame(parent);
        sep->setFrameShape(QFrame::VLine);
        sep->setStyleSheet("color: #2E5B80;");
        return sep;
    };

    auto makeLeg = [](QWidget* parent, const QString& texte, const QString& couleur) {
        QLabel* l = new QLabel(texte, parent);
        l->setStyleSheet(QString("color: %1; font-size: 10px;").arg(couleur));
        return l;
    };

    layoutInfo->addWidget(labelTauxSSS_SRS);
    layoutInfo->addWidget(makeSep(panneauInfo));
    layoutInfo->addWidget(labelTauxSRS_SDD);
    layoutInfo->addWidget(makeSep(panneauInfo));
    layoutInfo->addWidget(labelTauxGlobal);
    layoutInfo->addStretch();
    layoutInfo->addWidget(makeLeg(panneauInfo, "● Couvert",     "#27AE60"));
    layoutInfo->addWidget(makeLeg(panneauInfo, "● Non couvert", "#E74C3C"));
    layoutInfo->addWidget(makeLeg(panneauInfo, "● Lien brisé",  "#E67E22"));
    layoutInfo->addWidget(makeLeg(panneauInfo, "● Orpheline",   "#9B59B6"));

    // ── Panneau détail nœud ───────────────────────────────────────────────────
    panneauDetail = new QFrame(this);
    panneauDetail->setStyleSheet(R"(
        QFrame {
            background-color: #FFFFFF;
            border: 1px solid #CCDDEE;
            border-radius: 12px;
        }
    )");
    panneauDetail->setVisible(false);

    QVBoxLayout *layoutDetail = new QVBoxLayout(panneauDetail);
    layoutDetail->setContentsMargins(14, 12, 14, 12);
    layoutDetail->setSpacing(8);

    labelDetailTitre = new QLabel("Détails", panneauDetail);
    labelDetailTitre->setStyleSheet("color: #0D2B45; font-size: 13px; font-weight: bold;");
    labelDetailStats = new QLabel("", panneauDetail);
    labelDetailStats->setStyleSheet("color: #555555; font-size: 11px;");

    QLabel *labelListeTitre = new QLabel("Exigences non couvertes :", panneauDetail);
    labelListeTitre->setStyleSheet("color: #E74C3C; font-size: 10px; font-weight: bold;");

    listeDetailExigences = new QListWidget(panneauDetail);
    listeDetailExigences->setFixedHeight(110);
    listeDetailExigences->setStyleSheet(R"(
        QListWidget {
            background-color: #FFF8F8;
            border: 1px solid #FADBD8;
            border-radius: 6px;
            font-size: 10px;
        }
        QListWidget::item { padding: 4px 8px; border-bottom: 1px solid #FDECEA; }
        QListWidget::item:selected { background-color: #FADBD8; color: #E74C3C; }
    )");

    layoutDetail->addWidget(labelDetailTitre);
    layoutDetail->addWidget(labelDetailStats);
    layoutDetail->addWidget(labelListeTitre);
    layoutDetail->addWidget(listeDetailExigences);
}

// ─── resizeEvent ─────────────────────────────────────────────────────────────
void VueGrapheTracabilite::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    int filtresW = panneauFiltres->isVisible() ? 240 : 0;
    panneauFiltres->setGeometry(width() - 240, 0, 240, height());
    boutonFiltres->move(width() - 130, 12);
    if (panneauDetail->isVisible())
        panneauDetail->setGeometry(14, height() - 260, 380, 210);
}

// ─── toggleFiltres ───────────────────────────────────────────────────────────
void VueGrapheTracabilite::toggleFiltres()
{
    bool visible = panneauFiltres->isVisible();
    panneauFiltres->setVisible(!visible);
    boutonFiltres->setText(visible ? "⚙  Filtres ▶" : "⚙  Filtres ◀");
    resizeEvent(nullptr);
    update();
}

void VueGrapheTracabilite::onFiltreRapideChange(int index)
{
    filtreRapide = index;
    update();
}

// ─── nomCourtFichier ─────────────────────────────────────────────────────────
QString VueGrapheTracabilite::nomCourtFichier(const std::string& chemin) const
{
    size_t pos = chemin.find_last_of("/\\");
    if (pos != std::string::npos)
        return QString::fromStdString(chemin.substr(pos + 1));
    return QString::fromStdString(chemin);
}

bool VueGrapheTracabilite::noeudVisible(int index) const
{
    if (index < 0 || index >= (int)noeuds.size()) return false;
    const NoeudFichier &n = noeuds[index];
    if (filtreRapide == 1) return n.nbTotal - n.nbCouvertes > 0;
    if (filtreRapide == 2) return !liensBrises.isEmpty();
    return true;
}

// ─── construireGraphe ────────────────────────────────────────────────────────
void VueGrapheTracabilite::construireGraphe(const RapportTracabilite& rapport,
                                            const std::vector<ExigenceSRS>& srs,
                                            const std::vector<ExigenceSDD>& sdd,
                                            const std::vector<std::string>& orphelinesParam)
{
    noeuds.clear(); liens.clear(); liensBrises.clear(); this->orphelines.clear();

    for (const std::string& orph : orphelinesParam)
        this->orphelines.append(QString::fromStdString(orph));

    srsAnalyses = srs; sddAnalyses = sdd;
    rapportCourant = rapport;
    noeudSurvole = noeudSelectionne = lienSurvole = -1;
    filtreRapide = 0;

    for (const std::string &lb : rapport.liensBrises)
        liensBrises.append(QString::fromStdString(lb));

    QMap<QString, QList<const ExigenceSSS*>> sssParFichier;
    for (const auto& s : rapport.exigencesSSS)
        sssParFichier[nomCourtFichier(s.getFichierSource())].append(&s);

    QMap<QString, QList<const ExigenceSRS*>> srsParFichier;
    for (const auto& s : srs)
        srsParFichier[nomCourtFichier(s.getFichierSource())].append(&s);

    QMap<QString, QList<const ExigenceSDD*>> sddParFichier;
    for (const auto& d : sdd)
        sddParFichier[nomCourtFichier(d.getFichierSource())].append(&d);

    int nbSSS = sssParFichier.size();
    int nbSRS = srsParFichier.size();
    int nbSDD = sddParFichier.size();

    int xSSS = 140, xSRS = 460, xSDD = 780;
    int espacementV = 130;

    // ── Nœuds SSS ────────────────────────────────────────────────────────────
    int i = 0;
    for (auto it = sssParFichier.begin(); it != sssParFichier.end(); ++it, ++i) {
        const QString& nomF = it.key();
        const QList<const ExigenceSSS*>& exigs = it.value();
        int nbTotal = exigs.size();
        int nbD = 0, nbI = 0;
        QStringList cD, ncD, cI, ncI;

        for (const ExigenceSSS* p : exigs) {
            bool directe = false;
            for (const auto& s : srs) {
                const auto& t = s.getTracabilite();
                if (std::find(t.begin(), t.end(), p->getId()) != t.end()) { directe = true; break; }
            }
            if (directe) { nbD++; cD << QString::fromStdString(p->getId()); }
            else          { ncD << QString::fromStdString(p->getId()); }

            bool indirecte = false;
            for (const auto& s : srs) {
                const auto& t = s.getTracabilite();
                if (std::find(t.begin(), t.end(), p->getId()) != t.end()) {
                    for (const auto& d : sdd) {
                        const auto& r = d.getReferencedSRS();
                        if (std::find(r.begin(), r.end(), s.getId()) != r.end()) { indirecte = true; break; }
                    }
                    if (indirecte) break;
                }
            }
            if (indirecte) { nbI++; cI << QString::fromStdString(p->getId()); }
            else           { ncI << QString::fromStdString(p->getId()); }
        }

        NoeudFichier n;
        n.nomFichier = nomF; n.cheminComplet = exigs.first()->getFichierSource().c_str();
        n.type = SSS; n.couleur = COULEUR_SSS;
        n.nbTotal = nbTotal; n.nbCouvertes = nbD; n.nbCouvertesIndirect = nbI;
        n.pourcentage = nbTotal > 0 ? nbD * 100.0 / nbTotal : 0;
        n.pourcentageIndirect = nbTotal > 0 ? nbI * 100.0 / nbTotal : 0;
        n.exigencesCouvertes = cD; n.exigencesNonCouvertes = ncD;
        n.exigencesCouvertesIndirect = cI; n.exigencesNonCouvertesIndirect = ncI;
        int yStart = qMax(90, (height() - 50) / 2 - ((nbSSS - 1) * espacementV / 2));
        n.position = QPointF(xSSS, yStart + i * espacementV);
        noeuds.push_back(n);
    }

    // ── Nœuds SRS ────────────────────────────────────────────────────────────
    i = 0;
    for (auto it = srsParFichier.begin(); it != srsParFichier.end(); ++it, ++i) {
        const QString& nomF = it.key();
        const QList<const ExigenceSRS*>& exigs = it.value();
        int nbTotal = exigs.size();
        int nbD = 0, nbI = 0;
        QStringList cD, ncD, cI, ncI;

        for (const ExigenceSRS* p : exigs) {
            bool directe = false;
            for (const auto& d : sdd) {
                const auto& r = d.getReferencedSRS();
                if (std::find(r.begin(), r.end(), p->getId()) != r.end()) { directe = true; break; }
            }
            if (directe) { nbD++; cD << QString::fromStdString(p->getId()); }
            else          { ncD << QString::fromStdString(p->getId()); }

            bool indirecte = directe && !p->getTracabilite().empty();
            if (indirecte) { nbI++; cI << QString::fromStdString(p->getId()); }
            else           { ncI << QString::fromStdString(p->getId()); }
        }

        NoeudFichier n;
        n.nomFichier = nomF; n.cheminComplet = exigs.first()->getFichierSource().c_str();
        n.type = SRS; n.couleur = COULEUR_SRS;
        n.nbTotal = nbTotal; n.nbCouvertes = nbD; n.nbCouvertesIndirect = nbI;
        n.pourcentage = nbTotal > 0 ? nbD * 100.0 / nbTotal : 0;
        n.pourcentageIndirect = nbTotal > 0 ? nbI * 100.0 / nbTotal : 0;
        n.exigencesCouvertes = cD; n.exigencesNonCouvertes = ncD;
        n.exigencesCouvertesIndirect = cI; n.exigencesNonCouvertesIndirect = ncI;
        int yStart = qMax(90, (height() - 50) / 2 - ((nbSRS - 1) * espacementV / 2));
        n.position = QPointF(xSRS, yStart + i * espacementV);
        noeuds.push_back(n);
    }

    // ── Nœuds SDD ────────────────────────────────────────────────────────────
    i = 0;
    for (auto it = sddParFichier.begin(); it != sddParFichier.end(); ++it, ++i) {
        const QString& nomF = it.key();
        const QList<const ExigenceSDD*>& exigs = it.value();

        NoeudFichier n;
        n.nomFichier = nomF; n.cheminComplet = exigs.first()->getFichierSource().c_str();
        n.type = SDD; n.couleur = COULEUR_SDD;
        n.nbTotal = exigs.size(); n.nbCouvertes = exigs.size(); n.nbCouvertesIndirect = exigs.size();
        n.pourcentage = 100.0; n.pourcentageIndirect = 100.0;
        for (const ExigenceSDD* d : exigs)
            n.exigencesCouvertes << QString::fromStdString(d->getId());
        int yStart = qMax(90, (height() - 50) / 2 - ((nbSDD - 1) * espacementV / 2));
        n.position = QPointF(xSDD, yStart + i * espacementV);
        noeuds.push_back(n);
    }

    // ── Liens SRS → SSS ───────────────────────────────────────────────────────
    for (int iN = 0; iN < (int)noeuds.size(); iN++) {
        if (noeuds[iN].type != SRS) continue;
        const QString& nomSRS = noeuds[iN].nomFichier;
        QMap<QString, QSet<QString>> dejaD, dejaI;
        QMap<QString, int> covD, covI;

        for (const auto& s : srs) {
            if (nomCourtFichier(s.getFichierSource()) != nomSRS) continue;
            for (const std::string& ref : s.getTracabilite()) {
                QString refStr = QString::fromStdString(ref);
                for (const auto& sssEx : rapport.exigencesSSS) {
                    if (sssEx.getId() == ref) {
                        QString nomSSS = nomCourtFichier(sssEx.getFichierSource());
                        if (!dejaD[nomSSS].contains(refStr)) { dejaD[nomSSS].insert(refStr); covD[nomSSS]++; }
                        bool srsCouvSDD = false;
                        for (const auto& d : sdd) {
                            const auto& r = d.getReferencedSRS();
                            if (std::find(r.begin(), r.end(), s.getId()) != r.end()) { srsCouvSDD = true; break; }
                        }
                        if (srsCouvSDD && !dejaI[nomSSS].contains(refStr)) { dejaI[nomSSS].insert(refStr); covI[nomSSS]++; }
                        break;
                    }
                }
            }
        }

        for (auto it = covD.begin(); it != covD.end(); ++it) {
            const QString& nomSSS = it.key();
            int nbTot = 0;
            for (const auto& sssEx : rapport.exigencesSSS)
                if (nomCourtFichier(sssEx.getFichierSource()) == nomSSS) nbTot++;
            int idx = -1;
            for (int j = 0; j < (int)noeuds.size(); j++)
                if (noeuds[j].type == SSS && noeuds[j].nomFichier == nomSSS) { idx = j; break; }
            if (idx >= 0 && nbTot > 0) {
                LienFichier l;
                l.indexSource = iN; l.indexCible = idx;
                l.pourcentage = covD[nomSSS] * 100.0 / nbTot;
                l.pourcentageIndirect = covI.contains(nomSSS) ? covI[nomSSS] * 100.0 / nbTot : 0.0;
                l.estBrise = false;
                liens.push_back(l);
            }
        }
    }

    // ── Liens SDD → SRS ───────────────────────────────────────────────────────
    for (int iN = 0; iN < (int)noeuds.size(); iN++) {
        if (noeuds[iN].type != SDD) continue;
        const QString& nomSDD = noeuds[iN].nomFichier;
        QMap<QString, QSet<QString>> dejaD, dejaI;
        QMap<QString, int> covD, covI;

        for (const auto& d : sdd) {
            if (nomCourtFichier(d.getFichierSource()) != nomSDD) continue;
            for (const std::string& ref : d.getReferencedSRS()) {
                QString refStr = QString::fromStdString(ref);
                for (const auto& s : srs) {
                    if (s.getId() == ref) {
                        QString nomSRS = nomCourtFichier(s.getFichierSource());
                        if (!dejaD[nomSRS].contains(refStr)) { dejaD[nomSRS].insert(refStr); covD[nomSRS]++; }
                        bool traceeSSS = !s.getTracabilite().empty();
                        if (traceeSSS && !dejaI[nomSRS].contains(refStr)) { dejaI[nomSRS].insert(refStr); covI[nomSRS]++; }
                        break;
                    }
                }
            }
        }

        for (auto it = covD.begin(); it != covD.end(); ++it) {
            const QString& nomSRS = it.key();
            int nbTot = 0;
            for (const auto& s : srs)
                if (nomCourtFichier(s.getFichierSource()) == nomSRS) nbTot++;
            int idx = -1;
            for (int j = 0; j < (int)noeuds.size(); j++)
                if (noeuds[j].type == SRS && noeuds[j].nomFichier == nomSRS) { idx = j; break; }
            if (idx >= 0 && nbTot > 0) {
                LienFichier l;
                l.indexSource = iN; l.indexCible = idx;
                l.pourcentage = covD[nomSRS] * 100.0 / nbTot;
                l.pourcentageIndirect = covI.contains(nomSRS) ? covI[nomSRS] * 100.0 / nbTot : 0.0;
                l.estBrise = false;
                liens.push_back(l);
            }
        }
    }

    // ── Panneau info ──────────────────────────────────────────────────────────
    panneauInfo->setVisible(true);
    panneauDetail->setVisible(false);

    auto styleTaux = [](QLabel* lbl, const QString& texte, double val) {
        lbl->setText(texte);
        QString couleur = val >= 80 ? "#4DD9AC" : val >= 50 ? "#F0C060" : "#F07070";
        lbl->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(couleur));
    };

    styleTaux(labelTauxSSS_SRS,
              QString("SSS←SRS : %1%").arg(rapport.tauxSSS_SRS * 100, 0, 'f', 1),
              rapport.tauxSSS_SRS * 100);
    styleTaux(labelTauxSRS_SDD,
              QString("SRS←SDD : %1%").arg(rapport.tauxSRS_SDD * 100, 0, 'f', 1),
              rapport.tauxSRS_SDD * 100);
    styleTaux(labelTauxGlobal,
              QString("Global : %1%").arg(rapport.tauxGlobal * 100, 0, 'f', 1),
              rapport.tauxGlobal * 100);


    resizeEvent(nullptr);
    // Adapter la hauteur minimale pour permettre le scroll
    int nbNoeudsMax = qMax(qMax((int)sssParFichier.size(),
                                (int)srsParFichier.size()),
                                (int)sddParFichier.size());
    int hauteurMin = qMax(400, nbNoeudsMax * espacementV + 150);
    setMinimumHeight(hauteurMin);

    resizeEvent(nullptr);
    update();
}

// ─── paintEvent ──────────────────────────────────────────────────────────────
void VueGrapheTracabilite::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(niveauZoom, niveauZoom);

    QLinearGradient fond(0, 0, 0, height() / niveauZoom);
    fond.setColorAt(0, QColor("#EEF6FF"));
    fond.setColorAt(1, QColor("#E0EEF8"));
    painter.fillRect(QRectF(0, 0, width() / niveauZoom, height() / niveauZoom), fond);

    if (noeuds.empty()) {
        painter.setPen(QColor("#8AACCC"));
        QFont font; font.setPointSize(13); painter.setFont(font);
        painter.drawText(QRectF(0, 0, width() / niveauZoom, height() / niveauZoom),
                         Qt::AlignCenter,
                         "🔗  Lancez une analyse pour afficher le graphe de traçabilité");
        return;
    }

    painter.setPen(QPen(QColor("#B8D4EC"), 1, Qt::DashLine));
    painter.drawLine(QPointF(300, 56), QPointF(300, (height() - 50) / niveauZoom));
    painter.drawLine(QPointF(620, 56), QPointF(620, (height() - 50) / niveauZoom));

    QFont fontCol; fontCol.setPointSize(8); fontCol.setBold(true);
    painter.setFont(fontCol);
    painter.setPen(QColor("#6A9ABE"));
    painter.drawText(QRectF(0, 56, 300, 20), Qt::AlignCenter, "SSS");
    painter.drawText(QRectF(300, 56, 320, 20), Qt::AlignCenter, "SRS");
    painter.drawText(QRectF(620, 56, 300, 20), Qt::AlignCenter, "SDD");

    // ── Filtre par mot-clé ────────────────────────────────────────────────────
    QString recherche = filtreActif.filtreIdentifiant.trimmed().toLower();
    QSet<int> noeudsVisibles;

    if (recherche.isEmpty()) {
        for (int j = 0; j < (int)noeuds.size(); j++)
            noeudsVisibles.insert(j);
    } else {
        // Étape 1 : nœuds dont le nom contient le mot-clé
        QSet<int> noeudsMatches;
        for (int j = 0; j < (int)noeuds.size(); j++) {
            if (noeuds[j].nomFichier.toLower().contains(recherche))
                noeudsMatches.insert(j);
        }

        // Étape 2 : partir des nœuds matchés
        noeudsVisibles = noeudsMatches;

        // Étape 3 : ajouter les voisins directs des nœuds matchés uniquement
        for (int idx : noeudsMatches) {
            for (const auto& lien : liens) {
                if (lien.indexSource == idx) noeudsVisibles.insert(lien.indexCible);
                if (lien.indexCible  == idx) noeudsVisibles.insert(lien.indexSource);
            }
        }

        // Étape 4 : depuis les voisins directs, propager uniquement vers
        // des nœuds de TYPE DIFFÉRENT du nœud matché
        // Ex: SYST matche SSS → voisins = SRS → on propage vers SDD (≠ SSS) ✅
        //     mais pas vers d'autres SSS ou d'autres SRS ✅

        // Déterminer le(s) type(s) des nœuds matchés
        QSet<int> typesMatches;
        for (int idx : noeudsMatches)
            typesMatches.insert((int)noeuds[idx].type);

        QSet<int> voisinsAjoutes = noeudsVisibles - noeudsMatches;
        for (int idx : voisinsAjoutes) {
            for (const auto& lien : liens) {
                int candidat = -1;
                if (lien.indexSource == idx) candidat = lien.indexCible;
                if (lien.indexCible  == idx) candidat = lien.indexSource;
                if (candidat < 0 || noeudsVisibles.contains(candidat)) continue;

                // N'ajouter le candidat que s'il est d'un type différent du nœud matché
                if (!typesMatches.contains((int)noeuds[candidat].type))
                    noeudsVisibles.insert(candidat);
            }
        }
    } // ← accolade fermante du else recherche

    // ── Filtre pourcentage SSS ────────────────────────────────────────────────
    if (filtrePctActif) {
        QSet<int> aExclure;

        // Identifier les SSS hors de la plage [min, max]
        for (int j = 0; j < (int)noeuds.size(); j++) {
            if (noeuds[j].type != SSS) continue;
            double pct = vueIndirecte ? noeuds[j].pourcentageIndirect
                                      : noeuds[j].pourcentage;
            if (pct < filtrePctMin || pct > filtrePctMax)
                aExclure.insert(j);
        }

        // Retirer chaque SSS exclu + sa descendance (SRS + SDD liés)
        for (int idxSSS : aExclure) {
            noeudsVisibles.remove(idxSSS);

            // SRS directement liés à ce SSS
            QSet<int> srsLies;
            for (const auto& lien : liens) {
                if (lien.indexCible == idxSSS && noeuds[lien.indexSource].type == SRS)
                    srsLies.insert(lien.indexSource);
                if (lien.indexSource == idxSSS && noeuds[lien.indexCible].type == SRS)
                    srsLies.insert(lien.indexCible);
            }

            for (int idxSRS : srsLies) {
                // Garder ce SRS s'il est lié à un autre SSS visible
                bool autreSSS = false;
                for (const auto& lien : liens) {
                    if (lien.indexSource == idxSRS && noeuds[lien.indexCible].type == SSS
                        && !aExclure.contains(lien.indexCible)) { autreSSS = true; break; }
                    if (lien.indexCible == idxSRS && noeuds[lien.indexSource].type == SSS
                        && !aExclure.contains(lien.indexSource)) { autreSSS = true; break; }
                }
                if (autreSSS) continue;

                noeudsVisibles.remove(idxSRS);

                // SDD liés à ce SRS
                for (const auto& lien : liens) {
                    if (lien.indexSource == idxSRS && noeuds[lien.indexCible].type == SDD)
                        noeudsVisibles.remove(lien.indexCible);
                    if (lien.indexCible == idxSRS && noeuds[lien.indexSource].type == SDD)
                        noeudsVisibles.remove(lien.indexSource);
                }
            }
        }
    }

    // ── Dessin ────────────────────────────────────────────────────────────────
    for (int j = 0; j < (int)liens.size(); j++) {
        if (noeudsVisibles.contains(liens[j].indexSource) &&
            noeudsVisibles.contains(liens[j].indexCible))
            dessinerLien(painter, liens[j], j == lienSurvole);
    }

    for (int j = 0; j < (int)noeuds.size(); j++) {
        if (!noeudsVisibles.contains(j)) continue;
        dessinerNoeud(painter, noeuds[j], j == noeudSurvole, j == noeudSelectionne);
    }
}

// ─── dessinerNoeud ───────────────────────────────────────────────────────────
void VueGrapheTracabilite::dessinerNoeud(QPainter &painter,
                                         const NoeudFichier &noeud,
                                         bool survole, bool selectionne)
{
    double w = 160, h = 95;
    QRectF rect(noeud.position.x() - w/2, noeud.position.y() - h/2, w, h);
    QColor couleur = noeud.couleur;

    painter.setBrush(QColor(10, 30, 60, 30));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect.adjusted(5, 5, 5, 5), 14, 14);

    if (selectionne) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor("#F0B820"), 3));
        painter.drawRoundedRect(rect.adjusted(-3, -3, 3, 3), 16, 16);
    }

    QLinearGradient grad(rect.topLeft(), rect.bottomLeft());
    grad.setColorAt(0, couleur.lighter(115));
    grad.setColorAt(1, couleur);
    painter.setBrush(grad);
    painter.setPen(QPen(survole ? couleur.lighter(150) : couleur.darker(120),
                        survole ? 2 : 1));
    painter.drawRoundedRect(rect, 14, 14);

    QRectF bandeau(rect.x(), rect.y(), rect.width(), 26);
    painter.setBrush(couleur.darker(130));
    painter.setPen(Qt::NoPen);
    QPainterPath clipPath;
    clipPath.addRoundedRect(rect, 14, 14);
    painter.setClipPath(clipPath);
    painter.drawRect(bandeau);
    painter.setClipping(false);

    QString typeLabel = (noeud.type == SSS) ? "SSS" : (noeud.type == SRS) ? "SRS" : "SDD";
    QFont fontType; fontType.setPointSize(7); fontType.setBold(true);
    fontType.setLetterSpacing(QFont::AbsoluteSpacing, 2);
    painter.setFont(fontType);
    painter.setPen(QColor(255, 255, 255, 180));
    painter.drawText(QRectF(rect.x() + 8, rect.y() + 4, 40, 18), Qt::AlignVCenter, typeLabel);

    QString nomAffiche = noeud.nomFichier;
    if (nomAffiche.length() > 20) nomAffiche = nomAffiche.left(17) + "…";
    QFont fontNom; fontNom.setPointSize(8); fontNom.setBold(true);
    painter.setFont(fontNom);
    painter.setPen(Qt::white);
    painter.drawText(QRectF(rect.x() + 38, rect.y() + 4, rect.width() - 46, 18),
                     Qt::AlignVCenter | Qt::AlignLeft, nomAffiche);

    painter.setPen(QPen(QColor(255, 255, 255, 40), 1));
    painter.drawLine(QPointF(rect.x() + 8, rect.y() + 27),
                     QPointF(rect.x() + rect.width() - 8, rect.y() + 27));

    if (noeud.type == SDD) {
        QFont fontSDD; fontSDD.setPointSize(11); fontSDD.setBold(true);
        painter.setFont(fontSDD);
        painter.setPen(QColor("#0D2B45"));
        painter.drawText(QRectF(rect.x(), rect.y() + 30, rect.width(), 38),
                         Qt::AlignCenter,
                         QString("%1 exigence(s)").arg(noeud.nbTotal));
    } else {
        double pct    = vueIndirecte ? noeud.pourcentageIndirect : noeud.pourcentage;
        int nbCouv    = vueIndirecte ? noeud.nbCouvertesIndirect : noeud.nbCouvertes;
        int nbNonCouv = noeud.nbTotal - nbCouv;

        QFont fontPct; fontPct.setPointSize(20); fontPct.setBold(true);
        painter.setFont(fontPct);
        painter.setPen(Qt::white);
        painter.drawText(QRectF(rect.x(), rect.y() + 28, rect.width(), 38),
                         Qt::AlignCenter, QString("%1%").arg(pct, 0, 'f', 0));

        QFont fontStats; fontStats.setPointSize(7);
        painter.setFont(fontStats);
        painter.setPen(QColor(255, 255, 255, 190));
        painter.drawText(QRectF(rect.x() + 6, rect.y() + 68, rect.width() - 12, 16),
                         Qt::AlignCenter,
                         QString("✓ %1  ✗ %2  /  %3").arg(nbCouv).arg(nbNonCouv).arg(noeud.nbTotal));
    }

    if (survole) {
        QFont fontHint; fontHint.setPointSize(6); fontHint.setItalic(true);
        painter.setFont(fontHint);
        painter.setPen(QColor(255, 255, 255, 130));
        painter.drawText(QRectF(rect.x(), rect.y() + rect.height() - 14, rect.width(), 12),
                         Qt::AlignCenter, "Cliquez pour les détails");
    }
}

// ─── dessinerBadgePct ────────────────────────────────────────────────────────
void VueGrapheTracabilite::dessinerBadgePct(QPainter &painter, QPointF centre,
                                            double pct, QColor couleur)
{
    QString texte = QString("%1%").arg(pct, 0, 'f', 0);
    QFont fontBadge; fontBadge.setPointSize(9); fontBadge.setBold(true);
    painter.setFont(fontBadge);
    QFontMetrics fm(fontBadge);
    int tw = fm.horizontalAdvance(texte) + 16;
    int th = fm.height() + 8;

    painter.setBrush(couleur);
    painter.setPen(QPen(couleur.lighter(140), 1));
    painter.drawRoundedRect(QRectF(centre.x() - tw/2, centre.y() - th/2, tw, th), 8, 8);
    painter.setPen(Qt::white);
    painter.drawText(QRectF(centre.x() - tw/2, centre.y() - th/2, tw, th),
                     Qt::AlignCenter, texte);
}

// ─── dessinerLien ─────────────────────────────────────────────────────────────
void VueGrapheTracabilite::dessinerLien(QPainter &painter,
                                        const LienFichier &lien,
                                        bool survole)
{
    if (lien.indexSource < 0 || lien.indexSource >= (int)noeuds.size()) return;
    if (lien.indexCible  < 0 || lien.indexCible  >= (int)noeuds.size()) return;

    const NoeudFichier &source = noeuds[lien.indexSource];
    const NoeudFichier &cible  = noeuds[lien.indexCible];

    QColor couleur = vueIndirecte ? QColor("#3D3D3D") : COULEUR_SRS.lighter(130);
    double pct = vueIndirecte ? lien.pourcentageIndirect : lien.pourcentage;

    QPointF debut(source.position.x() - 80, source.position.y());
    QPointF fin(cible.position.x() + 80,    cible.position.y());

    painter.setPen(QPen(couleur, survole ? 3 : 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(debut, fin);

    double angle  = std::atan2(fin.y() - debut.y(), fin.x() - debut.x());
    double taille = 13.0;
    QPointF p1(fin.x() - taille * std::cos(angle - 0.38),
               fin.y() - taille * std::sin(angle - 0.38));
    QPointF p2(fin.x() - taille * std::cos(angle + 0.38),
               fin.y() - taille * std::sin(angle + 0.38));
    painter.setBrush(couleur);
    painter.setPen(Qt::NoPen);
    QPolygonF fleche; fleche << fin << p1 << p2;
    painter.drawPolygon(fleche);

    QPointF milieu((debut.x() + fin.x()) / 2, (debut.y() + fin.y()) / 2);
    dessinerBadgePct(painter, milieu, pct, couleur);
}

// ─── Interactions ─────────────────────────────────────────────────────────────
void VueGrapheTracabilite::mouseMoveEvent(QMouseEvent *event)
{
    QPointF pos = QPointF(event->pos()) / niveauZoom;
    int ancienNoeud = noeudSurvole, ancienLien = lienSurvole;
    noeudSurvole = noeudAPosition(pos);
    lienSurvole  = (noeudSurvole < 0) ? lienAPosition(pos) : -1;

    if (lienSurvole >= 0) {
        const LienFichier &l = liens[lienSurvole];
        double pct = vueIndirecte ? l.pourcentageIndirect : l.pourcentage;
        QToolTip::showText(event->globalPosition().toPoint(),
                           QString("<b>%1</b> → <b>%2</b><br/>Couverture : <b>%3%</b>")
                               .arg(noeuds[l.indexSource].nomFichier)
                               .arg(noeuds[l.indexCible].nomFichier)
                               .arg(pct, 0, 'f', 1), this);
    } else {
        QToolTip::hideText();
    }
    if (noeudSurvole != ancienNoeud || lienSurvole != ancienLien) update();
}

void VueGrapheTracabilite::mousePressEvent(QMouseEvent *event)
{
    QPointF pos = QPointF(event->pos()) / niveauZoom;
    int clique = noeudAPosition(pos);

    if (clique >= 0) {
        noeudSelectionne = (clique == noeudSelectionne) ? -1 : clique;
        if (noeudSelectionne >= 0) {
            const NoeudFichier &n = noeuds[noeudSelectionne];
            double pct  = vueIndirecte ? n.pourcentageIndirect : n.pourcentage;
            int nbCouv  = vueIndirecte ? n.nbCouvertesIndirect : n.nbCouvertes;
            const QStringList &nonCouv = vueIndirecte
                                             ? n.exigencesNonCouvertesIndirect
                                             : n.exigencesNonCouvertes;

            panneauDetail->setVisible(true);
            labelDetailTitre->setText(n.nomFichier);
            labelDetailStats->setText(
                QString("%1 exigences  |  ✓ %2 couvertes  |  ✗ %3 non couvertes  |  %4%")
                    .arg(n.nbTotal).arg(nbCouv)
                    .arg(n.nbTotal - nbCouv).arg(pct, 0, 'f', 1));

            listeDetailExigences->clear();
            if (nonCouv.isEmpty()) {
                QListWidgetItem *it = new QListWidgetItem("✓ Toutes les exigences sont couvertes !");
                it->setForeground(QColor("#27AE60"));
                listeDetailExigences->addItem(it);
            } else {
                for (const QString &id : nonCouv) {
                    QListWidgetItem *it = new QListWidgetItem("✗  " + id);
                    it->setForeground(QColor("#E74C3C"));
                    listeDetailExigences->addItem(it);
                }
            }
            resizeEvent(nullptr);
        } else {
            panneauDetail->setVisible(false);
        }
    } else {
        noeudSelectionne = -1;
        panneauDetail->setVisible(false);
    }
    update();
}

void VueGrapheTracabilite::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0) niveauZoom *= 1.1;
    else                              niveauZoom /= 1.1;
    niveauZoom = qBound(0.3, niveauZoom, 3.0);
    update();
}

int VueGrapheTracabilite::noeudAPosition(const QPointF &pos) const
{
    for (int j = 0; j < (int)noeuds.size(); j++) {
        double dx = pos.x() - noeuds[j].position.x();
        double dy = pos.y() - noeuds[j].position.y();
        if (std::abs(dx) <= 80 && std::abs(dy) <= 48) return j;
    }
    return -1;
}

int VueGrapheTracabilite::lienAPosition(const QPointF &pos) const
{
    for (int j = 0; j < (int)liens.size(); j++) {
        const NoeudFichier &src = noeuds[liens[j].indexSource];
        const NoeudFichier &cib = noeuds[liens[j].indexCible];
        QPointF milieu((src.position.x() - 80 + cib.position.x() + 80) / 2,
                       (src.position.y() + cib.position.y()) / 2);
        if (std::abs(pos.x() - milieu.x()) <= 32 && std::abs(pos.y() - milieu.y()) <= 20)
            return j;
    }
    return -1;
}

void VueGrapheTracabilite::appliquerFiltre(const FiltreGUI &filtre)
{
    filtreActif = filtre;
    update();
}

void VueGrapheTracabilite::setListeNomsFichiers(const QStringList& noms)
{
    gestionnaireFiltres->setListeIds(noms);
}

void VueGrapheTracabilite::setVueDirecte()
{
    vueIndirecte = false;
    update();
}

void VueGrapheTracabilite::setVueIndirecte()
{
    vueIndirecte = true;
    update();
}
