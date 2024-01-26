#include "Timer.h"
#include <iostream>
#include <fstream>

Timer::Timer( std::string msg )
{
    // Rediriger les erreurs dans ce fichier
    exceptionLog_ = freopen( "C:\\error.log", "w", stderr );
    char buf[BUFSIZ];
    setvbuf( stderr, buf, _IOLBF, BUFSIZ );
    
    msg_ = msg;
    time_started_ = std::chrono::high_resolution_clock::now();
}

Timer::~Timer()
{
    pause();
    auto d = std::chrono::duration<double>( duration_ ).count();
    std::cout << msg_ << " :" << std::fixed << d << " secondes." << std::endl;
    
    std::ofstream errfile( "D:\\error.txt" );
    
    if( errfile )
    {
        errfile << stderr << std::endl;
        errfile.close();
    }
    
    if( exceptionLog_ )
        fclose( exceptionLog_ );
        
    std::remove( "error.log" );
}

void Timer::resume()
{
    if( !paused_ ) return;
    
    paused_ = false;
    time_started_ = std::chrono::high_resolution_clock::now();
}

void Timer::pause()
{
    if( paused_ ) return;
    
    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    duration_ += ( now - time_started_ );
    paused_ = true;
}