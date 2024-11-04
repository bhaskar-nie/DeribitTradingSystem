#ifndef API_H
#define API_H

#include <string>
#include <vector>
#include <unordered_set>
#include "json.hpp" 
#include "InstrumentManager.h" 

using json = nlohmann::json;

std::string getAccessToken(const std::string& clientId, const std::string& clientSecret);
void fetchInstruments(const std::string& currency, const std::string& kind, const std::string& accessToken, InstrumentManager& instrumentManager);
void placeBuyOrder(const std::string& currency, const std::string& kind, const std::string& symbol, double amount, const std::string& label, const std::string& type, const std::string& accessToken);
void displayBuyOrderResponse(const json& responseJson);
void placeSellOrder(const std::string& currency, const std::string& kind, const std::string& symbol, double amount, double price, const std::string& trigger, double triggerPrice, const std::string& type, const std::string& accessToken);
void displaySellOrderResponse(const json& responseJson);
void cancelOrder(const std::string& orderId, const std::string& accessToken);
void cancelAllOrders(const std::string& accessToken);
void cancelAllByCurrency(const std::string& currency, const std::string& kind, const std::string& accessToken);
void cancelAllByCurrencyPair(const std::string& currencyPair, const std::string& kind, const std::string& accessToken);
void cancelAllByInstrument(const std::string& instrumentName, const std::string& type, const std::string& accessToken);
void cancelAllByKindOrType(const std::vector<std::string>& currencies, const std::string& kind, const std::string& accessToken);
void cancelByLabel(const std::string& label, const std::string& accessToken);
void editOrder(const std::string& orderId, double amount, double price, const std::string& accessToken);
void editOrderByLabel(const std::string& label, double amount, double price, const std::string& instrumentName, const std::string& accessToken);
void getOrderBook2(const std::string& instrument_name);
std::string getOrderBook(const std::string& instrument_name); 
void getOrderBookByInstrumentId(int instrument_id);
size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
void getPositionByInstrumentName(const std::string& instrument_name, const std::string& accessToken);
void getAllPositions(const std::string& currency, const std::string& kind, const std::string& accessToken);

#endif 
