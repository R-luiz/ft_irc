// Logger.cpp
#include "Logger.hpp"
#include <iostream>

std::ofstream Logger::logFile;

void Logger::init(const std::string& filename) {
    logFile.open(filename.c_str(), std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

void Logger::log(const std::string& message) {
    if (logFile.is_open()) {
        logFile << message << std::endl;
    }
    std::cout << message << std::endl;
}

void Logger::cleanup() {
    if (logFile.is_open()) {
        logFile.close();
    }
}
