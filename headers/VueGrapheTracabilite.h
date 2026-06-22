#ifndef VUEGRAPHETRACABILITE_H
#define VUEGRAPHETRACABILITE_H
/**
 * @file VueGrapheTracabilite.h
 * @brief Vue graphe de traçabilité — design amélioré avec 2 vues.
 * @version 7.0
 */
#include <QWidget>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QFrame>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QComboBox>
#include <QStringList>
#include <QMap>
#include <vector>
#include <string>
#include <QSlider>
#include "RapportTracabilite.h"
#include "ExigenceSRS.h"
#include "ExigenceSDD.h"
#include "GestionnaireFiltresGUI.h"

struct NoeudFichier {
    QString      nomFichier;
    QString      cheminComplet;
    TypeExigence type;
    QPointF      position;
    QColor       couleur;
    int          nbTotal;
    int          nbCouvertes;
    int          nbCouvertesIndirect;
    double       pourcentage;
    double       pourcentageIndirect;
    QStringList  exigencesCouvertes;
    QStringList  exigencesNonCouvertes;
    QStringList  exigencesCouvertesIndirect;
    QStringList  exigencesNonCouvertesIndirect;
};

struct LienFichier {
    int    indexSource;
    int    indexCible;
    double pourcentage;
    double pourcentageIndirect;
    bool   estBrise;
};

class VueGrapheTracabilite : public QWidget
{
    Q_OBJECT
public:
    explicit VueGrapheTracabilite(QWidget *parent = nullptr);

    void construireGraphe(const RapportTracabilite& rapport,
                          const std::vector<ExigenceSRS>& srs,
                          const std::vector<ExigenceSDD>& sdd,
                          const std::vector<std::string>& orphelinesParam);

    void setListeNomsFichiers(const QStringList& noms);
    void setVueDirecte();
    void setVueIndirecte();
    QFrame* getPanneauInfo() { return panneauInfo; }

public slots:
    void appliquerFiltre(const FiltreGUI& filtre);
    void toggleFiltres();
    void onFiltreRapideChange(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    std::vector<NoeudFichier>  noeuds;
    std::vector<LienFichier>   liens;
    QStringList                liensBrises;
    QStringList                orphelines;
    RapportTracabilite         rapportCourant;
    std::vector<ExigenceSRS>   srsAnalyses;
    std::vector<ExigenceSDD>   sddAnalyses;

    bool      vueIndirecte;
    int       filtreRapide;
    FiltreGUI filtreActif;
    double    niveauZoom;
    int       noeudSurvole;
    int       noeudSelectionne;
    int       lienSurvole;
    // Curseurs % SSS — propres au graphe
    int       filtrePctMin;
    int       filtrePctMax;
    bool      filtrePctActif;
    QSlider  *sliderPctMin;
    QSlider  *sliderPctMax;
    QLabel   *labelPctMin;
    QLabel   *labelPctMax;

    // Couleurs des 3 niveaux
    static const QColor COULEUR_SSS; // bleu marine foncé
    static const QColor COULEUR_SRS; // bleu moyen
    static const QColor COULEUR_SDD; // bleu clair


    QComboBox              *comboFiltreRapide;
    QFrame                 *panneauFiltres;
    QPushButton            *boutonFiltres;
    GestionnaireFiltresGUI *gestionnaireFiltres;
    QFrame                 *panneauInfo;
    QLabel                 *labelTauxSSS_SRS;
    QLabel                 *labelTauxSRS_SDD;
    QLabel                 *labelTauxGlobal;
    QLabel                 *labelLiensBrises;
    QFrame                 *panneauDetail;
    QLabel                 *labelDetailTitre;
    QLabel                 *labelDetailStats;
    QListWidget            *listeDetailExigences;

    void dessinerNoeud(QPainter &painter, const NoeudFichier &noeud,
                       bool survole, bool selectionne);
    void dessinerLien(QPainter &painter, const LienFichier &lien, bool survole);
    void dessinerBadgePct(QPainter &painter, QPointF centre, double pct, QColor couleur);

    int     noeudAPosition(const QPointF &pos) const;
    int     lienAPosition(const QPointF &pos) const;
    bool    noeudVisible(int index) const;
    QString nomCourtFichier(const std::string& chemin) const;
    void    styleBoutonActif(QPushButton* btn, QColor couleur);
    void    styleBoutonInactif(QPushButton* btn);
};

#endif // VUEGRAPHETRACABILITE_H
