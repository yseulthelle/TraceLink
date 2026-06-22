#ifndef MAINWINDOW_H
#define MAINWINDOW_H
/**
 * @file mainwindow.h
 * @brief Fenêtre principale de TraceLink — Module 3.
 * @version 8.1 — Préfixe configurable par fichier + améliorations design
 */
#include <QMainWindow>
#include <QTabWidget>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QStringList>
#include <QTimer>
#include <QMap>
#include "OngletRapport.h"
#include "VueGrapheTracabilite.h"
#include "GestionnaireFiltresGUI.h"
#include "ExtracteurFichier.h"
#include "ConvertisseurExigence.h"
#include "MoteurTracabilite.h"
#include "ThreadManager.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void parcourirSSS();
    void parcourirSRS();
    void parcourirSDD();
    void verifierFichiersSelectionnes();
    void reinitialiser();
    void lancerAnalyse();
    void filtrerApercu(const QString &texte);

private:
    // Onglets principaux
    QTabWidget             *tabWidget;
    QWidget                *ongletImport;
    OngletRapport          *ongletRapport;
    VueGrapheTracabilite   *ongletGraphe;
    GestionnaireFiltresGUI *gestionnaireFiltres;
    QWidget                *ongletGrapheConteneur;
    QWidget                *ongletRapportConteneur;

    // Onglets fichiers SSS/SRS/SDD
    QTabWidget  *ongletsFichiers;
    QLineEdit   *champSSS;
    QLineEdit   *champSRS;
    QLineEdit   *champSDD;
    QPushButton *btnParcourirSSS;
    QPushButton *btnParcourirSRS;
    QPushButton *btnParcourirSDD;
    QLabel      *badgeSSS;
    QLabel      *badgeSRS;
    QLabel      *badgeSDD;
    QLabel      *erreurSSS;
    QLabel      *erreurSRS;
    QLabel      *erreurSDD;
    QListWidget *listeSSS;
    QListWidget *listeSRS;
    QListWidget *listeSDD;

    // Frames dynamiques pour les préfixes par fichier
    QFrame      *framesPrefixesSSS;
    QVBoxLayout *layoutPrefixesSSS;
    QFrame      *framesPrefixesSRS;
    QVBoxLayout *layoutPrefixesSRS;
    QFrame      *framesPrefixesSDD;
    QVBoxLayout *layoutPrefixesSDD;

    // Map chemin → champ préfixe pour chaque type
    QMap<QString, QLineEdit*> prefixesParFichierSSS;
    QMap<QString, QLineEdit*> prefixesParFichierSRS;
    QMap<QString, QLineEdit*> prefixesParFichierSDD;

    // Boutons + statut
    QPushButton *btnLancer;
    QPushButton *btnReinitialiser;
    QLabel      *labelStatut;

    // Terminal + progression
    QTextEdit    *terminal;
    QProgressBar *barreProgression;

    // Aperçu exigences
    QListWidget *listeExigences;
    QLineEdit   *champRechercheId;
    QLabel      *labelNbExigences;
    QStringList  toutesExigences;

    // Chemins — listes pour sélection multiple
    QStringList cheminsSSS;
    QStringList cheminsSRS;
    QStringList cheminsSDD;

    // Thread
    MoteurTracabilite *moteurAnalyse = nullptr;
    ThreadManager     *threadManager = nullptr;

    // Méthodes
    void construireOngletImport();
    void construireOngletGraphe();
    void construireOngletRapport();
    bool formatAccepte(const QString &chemin);
    void mettreAJourOngletFichier(int index, const QString &etat);
    void logTerminal(const QString &message, const QString &couleur = "#D4D4D4");
    void afficherApercuExigences(const std::vector<ExigenceExtraite> &sss,
                                 const std::vector<ExigenceExtraite> &srs,
                                 const std::vector<ExigenceExtraite> &sdd);
    void creerChampsPrefixes(const QStringList &chemins,
                             QVBoxLayout *layout,
                             QMap<QString, QLineEdit*> &map,
                             const QString &placeholder);
    /// @brief Ajoute l'item "Aucun fichier sélectionné" par défaut dans une liste.
    void ajouterItemVide(QListWidget *liste);
};
#endif // MAINWINDOW_H
