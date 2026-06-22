/*
 * ThreadManager.cpp
 *
 *  Created on: 23 mars 2026
 */


#include "ThreadManager.h"
#include <iostream>

//Constructeur
ThreadManager::ThreadManager(MoteurTracabilite& moteur)
    : moteur(moteur), analyseTerminee(false) {}

//si l'objet est détruit alors que le thread tourne encore, on le joint
//sinon : std::threda appelerait stdd::terminate() au momenet de sa destruction
ThreadManager::~ThreadManager(){
	if(threadAnalyse.joinable()){
		threadAnalyse.join();
	}
}


// Lance l'analyse dans un thread séparé
void ThreadManager::lancerAnalyse() {

	//évite de lancer un deuxième thread si un est déjà actif
	if(threadAnalyse.joinable()){
		return;
	}

    analyseTerminee = false;

    threadAnalyse = std::thread([this]() { //l'analyse démarre en parrallèle dans un thread séparé

        std::cout << "Analyse en cours..." << std::endl;

        moteur.analyserTracabilite();
        moteur.calculerTaux();

        analyseTerminee = true;

        std::cout << "Analyse terminée" << std::endl;
    });
}

//attend que le thread soit complètement fini
void ThreadManager::attendreFinAnalyse() { //bloque le programme principal jusqu'à ce que le thread ait fini
    if (threadAnalyse.joinable()) { //vérifie que le thread existe bien
        threadAnalyse.join(); //=on attend que ce thread soit terminé avant de continuer
    }
}

//retourne l'état de l'analyse
bool ThreadManager::estTerminee() const { //permet de vérifier l'état de l'analyse sans bloquer le programme
    return analyseTerminee;
}


