#ifndef ACCUEIL_H
#define ACCUEIL_H
#include <QWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QLabel> // pour labelChargement

/**
 * @brief Fenêtre de démarrage de TraceLink.
 * Affiche le nom du logiciel, une barre de chargement et un bouton Démarrer.
 */
class Accueil: public QWidget
{
    Q_OBJECT
public:
    explicit Accueil(QWidget *parent = nullptr);
signals:
    /// @brief Émis quand l'utilisateur clique sur Démarrer.
    void demarrerClique();
private slots:
    void avancerChargement();
private:
    QProgressBar *barreChargement;
    QPushButton  *btnDemarrer;
    QTimer       *timer;
    int           progression;
    QLabel       *labelChargement; // label animé de chargement
    void setupUI();
};
#endif // ACCUEIL_H
