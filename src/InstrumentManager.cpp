#include "InstrumentManager.h"
#include <iostream>

void InstrumentManager::addInstrument(const std::string& instrumentName) {
    std::lock_guard<std::mutex> lock(mtx); 
    if (instruments.insert(instrumentName).second) {
        std::cout << "Instrument added: " << instrumentName << std::endl;
    } else {
        std::cout << "Instrument already exists: " << instrumentName << std::endl;
    }
}

bool InstrumentManager::isSymbolSupported(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(mtx);
    return instruments.find(symbol) != instruments.end();
}
