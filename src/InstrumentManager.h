#ifndef INSTRUMENT_MANAGER_H
#define INSTRUMENT_MANAGER_H

#include <string>
#include <set>
#include <mutex>

class InstrumentManager {
public:
    void addInstrument(const std::string& instrumentName);
    bool isSymbolSupported(const std::string& symbol) const;

private:
    std::set<std::string> instruments;
    mutable std::mutex mtx;
};

#endif 
