#pragma once
#pragma warning(disable: 4996)
#include <string>
#include <chrono>

class Timer {
    public:
        /// Constructeur + texte à afficher à la fin de l'opération
        Timer( std::string msg = "Execution time" );
        
        /// Destructeur
        ~Timer();
        
        /// @brief: Méthode permettant de reprendre le compteur
        /// @param: void
        /// @return: void
        void resume();
        
        /// @brief: Méthode permettant de pauser le timer
        /// @param: void
        /// @return: void
        void pause();
    private:
        // Temps on on a instancié le timer
        std::chrono::high_resolution_clock::time_point time_started_ = std::chrono::high_resolution_clock::now();
        
        // Durée jusqu'à la destruction du timer
        std::chrono::high_resolution_clock::duration duration_ = {};
        bool paused_ = false;
        
        // Message à afficher
        std::string msg_;
        
        // Pour rediriger les exceptions qu'on ne veut pas afficher
        FILE* exceptionLog_ = NULL;
};
