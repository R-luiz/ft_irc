// Logger.hpp
#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>

class Logger {
private:
    static std::ofstream logFile;

public:
    static void init(const std::string& filename);
    static void log(const std::string& message);
    static void cleanup();
};

#endif

