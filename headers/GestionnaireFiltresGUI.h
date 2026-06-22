/**
 * @file GestionnaireFiltresGUI.h
 * @brief Déclaration de la classe GestionnaireFiltresGUI.
 * @date 02/04/2026
 * @version 1.0
 */
#ifndef GESTIONNAIREFILTRESGUI_H
#define GESTIONNAIREFILTRESGUI_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QCompleter>
#include <QStringListModel>
#include <QSlider>
#include <QSpinBox>
#include "Exigence.h"

// MODIF : TypeDoc conservé pour compatibilité mais filtre document retiré de l'UI
enum TypeDoc {
    TOUS_DOCS = 0,
    DOC_SSS,
    DOC_SRS,
    DOC_SDD
};

struct FiltreGUI {
    TypeDoc  filtreDocument    = TOUS_DOCS;
    Statut   filtreStatut      = NonAnalyse;
    QString  filtreIdentifiant = "";
    bool     filtreDevEtat     = false;
    bool     filtreDevValeur   = false;
    bool     filtreOrpheline   = false;
    bool     filtreLienBrise   = false;
    bool     filtreEnCours     = false;
};
class GestionnaireFiltresGUI : public QWidget
{
    Q_OBJECT

public:
    explicit GestionnaireFiltresGUI(QWidget *parent = nullptr, bool modeGraphe = false);
    FiltreGUI getFiltreActuel() const;
    void setFiltreStatut(int index);
    void setFiltreIdentifiant(const QString& texte);
    void setListeIds(const QStringList& ids);

signals:
    void filtreModifie(const FiltreGUI& filtre);

public slots:
    void reinitialiser();

private slots:
    void appliquerFiltres();

private:
    void initialiserUI();
    QLabel* creerLabel(const QString& texte);

    QComboBox   *comboStatut;
    QComboBox   *comboEtat;
    QLineEdit   *champIdentifiant;
    QPushButton *btnAppliquer;
    QPushButton *btnReinitialiser;
    QCompleter  *completer;
    QStringList  listeIds;
    FiltreGUI    filtreCourant;
    bool         modeGraphe;
};


#endif // GESTIONNAIREFILTRESGUI_H
