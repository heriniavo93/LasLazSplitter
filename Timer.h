#pragma once
#pragma warning(disable: 4996)
#include <string>
#include <chrono>

class Timer {
    public:
        /// Constructeur + texte � afficher � la fin de l'op�ration
        Timer( std::string msg = "Execution time" );
        
        /// Destructeur
        ~Timer();
        
        /// @brief: M�thode permettant de reprendre le compteur
        /// @param: void
        /// @return: void
        void resume();
        
        /// @brief: M�thode permettant de pauser le timer
        /// @param: void
        /// @return: void
        void pause();
    private:
        // Temps on on a instanci� le timer
        std::chrono::high_resolution_clock::time_point time_started_ = std::chrono::high_resolution_clock::now();
        
        // Dur�e jusqu'� la destruction du timer
        std::chrono::high_resolution_clock::duration duration_ = {};
        bool paused_ = false;
        
        // Message � afficher
        std::string msg_;
        
        // Pour rediriger les exceptions qu'on ne veut pas afficher
        FILE* exceptionLog_ = NULL;
};
