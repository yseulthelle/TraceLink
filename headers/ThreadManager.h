/*
 * ThreadManager.h
 *
 *  Created on: 23 mars 2026
 */

#ifndef THREADMANAGER_H_
#define THREADMANAGER_H_

#include "MoteurTracabilite.h"
#include <thread>
#include <functional>
#include <atomic>

class ThreadManager {

private:
    MoteurTracabilite& moteur; //le & nous permet de travailler sur le moteur, et non sur une copie -> permettera au thread de modifier le moteur
    std::thread threadAnalyse; //le thread d'arrière-plan qui va lancer l'analyse
    std::atomic<bool> analyseTerminee; //booléen qui permettera de savoir si l'analyse est terminee

public:
    //Constructeur
    ThreadManager(MoteurTracabilite& moteur); //le moteur est déjà rempli par setDonnees()
    ~ThreadManager(); //destructeur pour joindre le thread automatiquement si nécessaire

    //Méthodes
    //lance le thread d'analyse en arrière-plan
    void lancerAnalyse();

    //attend que le thread soit fini avant de continuer
    void attendreFinAnalyse();

    //aetourne true si l'analyse est terminée
    bool estTerminee() const;
};

#endif /* THREADMANAGER_H_ */
