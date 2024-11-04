#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>
#include "api.h" 
#include "websocket_server.h" 
#include "websocket_client.cpp"

using namespace std;
using json = nlohmann::json;

void displayMenu() {
    std::cout << "Please select an option:\n";
    std::cout << "1. Place Buy Order\n";
    std::cout << "2. Place Sell Order\n";
    std::cout << "3. Modify Order\n";
    std::cout << "4. Get Order Book\n";
    std::cout << "5. View Current Positions\n";
    std::cout << "6. Start WebSocket Server\n"; 
    std::cout << "7. Subscribe to Symbol for Order Book Updates\n";
    std::cout << "0. Exit\n";
}

void displayModifyMenu() {
    cout << "Modify Order - Choose an option:\n";
    cout << "1. Cancel Order by ID\n";
    cout << "2. Cancel All Orders\n";
    cout << "3. Cancel Orders by Currency\n";
    cout << "4. Cancel Orders by Currency Pair\n";
    cout << "5. Cancel Orders by Instrument\n";
    cout << "6. Cancel Orders by Label\n";
    cout << "7. Cancel Orders by Kind or Type\n";
    cout << "8. Edit Order by ID\n";           
    cout << "9. Edit Order by Label\n";        
    cout << "0. Go Back\n";
}

std::pair<std::string, std::string> loadApiKeys(const std::string& configPath) {
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        cerr << "Could not open config file\n";
        return {"", ""};
    }

    json configJson;
    configFile >> configJson;
    std::string clientId = configJson["clientId"];
    std::string clientSecret = configJson["clientSecret"];
    return {clientId, clientSecret};
}

int main() {
    WebSocketClient wsClient;

    auto [clientId, clientSecret] = loadApiKeys("config.json");
    if (clientId.empty() || clientSecret.empty()) {
        cerr << "API keys are missing. Exiting.\n";
        return 1;
    }

    std::string accessToken = getAccessToken(clientId, clientSecret);
    if (accessToken.empty()) {
        std::cerr << "Failed to retrieve access token. Exiting.\n";
        return 1;
    }

    WebSocketServer wsServer;
    std::thread wsThread; 

    int choice;
    while (true) {
        displayMenu();
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1: { 
                std::string currency, kind, symbol, label;
                double amount;
                std::string type = "market"; 

                std::cout << "Enter currency (e.g., BTC, ETH): ";
                std::cin >> currency;
                std::cout << "Enter kind (e.g., future, option): ";
                std::cin >> kind;

                InstrumentManager instrumentManager;

                std::cout << "Fetching available " << currency << " " << kind << "s...\n";
                fetchInstruments(currency, kind, accessToken, instrumentManager); 

                std::cout << "Enter symbol (e.g., BTC-PERPETUAL): ";
                std::cin >> symbol;

                if (!instrumentManager.isSymbolSupported(symbol)) {
                    std::cerr << "Error: Symbol " << symbol << " is not supported.\n";
                    break; 
                }

                std::cout << "Enter amount: ";
                std::cin >> amount;
                std::cout << "Enter label: ";
                std::cin >> label;

                placeBuyOrder(currency, kind, symbol, amount, label, type, accessToken);
                break;
            }

            case 2: { 
                std::string currency, kind, symbol, trigger, type;
                double amount, price, triggerPrice;

                std::cout << "Enter currency (e.g., BTC, ETH): ";
                std::cin >> currency;
                std::cout << "Enter kind (e.g., future, option): ";
                std::cin >> kind;

                InstrumentManager instrumentManager;

                std::cout << "Fetching available " << currency << " " << kind << "s...\n";
                fetchInstruments(currency, kind, accessToken, instrumentManager); 

                std::cout << "Enter symbol (e.g., BTC-PERPETUAL): ";
                std::cin >> symbol;

                if (!instrumentManager.isSymbolSupported(symbol)) {
                    std::cerr << "Error: Symbol " << symbol << " is not supported.\n";
                    break; 
                }

                std::cout << "Enter amount: ";
                std::cin >> amount;
                std::cout << "Enter price: ";
                std::cin >> price;
                std::cout << "Enter trigger type (e.g., last_price): ";
                std::cin >> trigger;
                std::cout << "Enter trigger price: ";
                std::cin >> triggerPrice;
                std::cout << "Enter order type (e.g., stop_limit): ";
                std::cin >> type;

                placeSellOrder(currency, kind, symbol, amount, price, trigger, triggerPrice, type, accessToken);
                break;
            }

            case 3: { 
                int modifyChoice;
                while (true) {
                    displayModifyMenu();
                    cout << "Enter your choice: ";
                    cin >> modifyChoice;

                    switch (modifyChoice) {
                        case 1: { 
                            string orderId;
                            cout << "Enter Order ID to cancel: ";
                            cin >> orderId;
                            cancelOrder(orderId, accessToken); 
                            break;
                        }
                        case 2: { 
                            cancelAllOrders(accessToken);  
                            break;
                        }
                        case 3: { 
                            string currency, kind;
                            cout << "Enter currency (e.g., BTC): ";
                            cin >> currency;
                            cout << "Enter kind (e.g., option or future): ";
                            cin >> kind;
                            cancelAllByCurrency(currency, kind, accessToken);  
                            break;
                        }
                        case 4: { 
                            string currencyPair, kind;
                            cout << "Enter currency pair (e.g., BTC_USD): ";
                            cin >> currencyPair;
                            cout << "Enter kind (e.g., option or future): ";
                            cin >> kind;
                            cancelAllByCurrencyPair(currencyPair, kind, accessToken); 
                            break;
                        }
                        case 5: { 
                            string instrumentName, type;
                            cout << "Enter instrument name (e.g., ETH-22FEB19-120-P): ";
                            cin >> instrumentName;
                            cout << "Enter order type (e.g., all): ";
                            cin >> type;
                            cancelAllByInstrument(instrumentName, type, accessToken); 
                            break;
                        }
                        case 6: { 
                            string label;
                            cout << "Enter label to cancel orders by: ";
                            cin >> label;
                            cancelByLabel(label, accessToken); 
                            break;
                        }
                        case 7: { 
                            std::string kind;
                            std::vector<std::string> currencies;

                            std::string input;
                            std::cout << "Enter supported currencies (separated by spaces, e.g., BTC ETH): ";
                            std::getline(std::cin >> std::ws, input);  

                            std::istringstream iss(input);
                            while (iss >> input) {
                                currencies.push_back(input);
                            }

                            std::cout << "Select kind of orders to cancel:\n";
                            std::cout << "1. Spot\n";
                            std::cout << "2. Futures\n";
                            std::cout << "3. Options\n";
                            std::cout << "Please enter your choice: ";
                            
                            int choice;
                            std::cin >> choice;
                            
                            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                            switch (choice) {
                                case 1:
                                    kind = "spot";
                                    break;
                                case 2:
                                    kind = "future";
                                    break;
                                case 3:
                                    kind = "option";
                                    break;
                                default:
                                    std::cerr << "Invalid choice. Please select 1, 2, or 3." << std::endl;
                                    break; 
                            }

                            if (!kind.empty()) {
                                cancelAllByKindOrType(currencies, kind, accessToken);
                            }
                            break;
                        }


                        case 8: {
                            string orderId;
                            double newAmount, newPrice;

                            cout << "Enter Order ID to edit: ";
                            cin >> orderId;

                            cout << "Enter new amount: ";
                            cin >> newAmount;

                            cout << "Enter new price: ";
                            cin >> newPrice;

                            editOrder(orderId, newAmount, newPrice, accessToken);
                            break;
                        }

                        case 9: { 
                            string label, instrumentName;
                            double amount, price;
                            cout << "Enter Label to edit: ";
                            cin >> label;
                            cout << "Enter new amount: ";
                            cin >> amount;
                            cout << "Enter new price: ";
                            cin >> price;
                            cout << "Enter Instrument Name: ";
                            cin >> instrumentName;
                            editOrderByLabel(label, amount, price, instrumentName, accessToken);
                            break;
                        }
                        
                        case 0: 
                            cout << "Returning to main menu.\n";
                            goto endModifyMenu;
                        default:
                            cout << "Invalid choice. Please try again.\n";
                    }
                }
                endModifyMenu:
                break;
            }

            case 4: { 
                int orderBookChoice;
                while (true) {
                    std::cout << "\nGet Order Book Menu:\n";
                    std::cout << "1. Get Order Book by Instrument Name\n";
                    std::cout << "2. Get Order Book by Instrument ID\n";
                    std::cout << "0. Go back to Main Menu\n";
                    std::cout << "Enter your choice: ";
                    std::cin >> orderBookChoice;

                    switch (orderBookChoice) {
                        case 1: { 
                            std::string instrumentName;
                            std::cout << "Enter Instrument Name (e.g., BTC-PERPETUAL): ";
                            std::cin >> instrumentName;
                            getOrderBook2(instrumentName);
                            break;
                        }
                        case 2: { 
                            int instrumentId;
                            std::cout << "Enter Instrument ID: ";
                            std::cin >> instrumentId;
                            getOrderBookByInstrumentId(instrumentId);
                            break;
                        }
                        case 0: 
                            std::cout << "Returning to main menu.\n";
                            goto endOrderBookMenu;
                        default:
                            std::cout << "Invalid choice. Please try again.\n";
                    }
                }
                endOrderBookMenu:
                break;
            }

            case 5: { 
                std::cout << "Select option:\n";
                std::cout << "1. Get Position by Instrument Name\n";
                std::cout << "2. Get All Positions\n";
                int subChoice;
                std::cin >> subChoice;

                if (subChoice == 1) {
                    std::string instrument_name;
                    std::cout << "Enter instrument name (e.g., BTC-PERPETUAL): ";
                    std::cin >> instrument_name;
                    getPositionByInstrumentName(instrument_name, accessToken); 
                } else if (subChoice == 2) {
                    std::string currency, kind;
                    std::cout << "Enter currency (e.g., BTC): ";
                    std::cin >> currency;
                    std::cout << "Enter kind (e.g., future): ";
                    std::cin >> kind;
                    getAllPositions(currency, kind, accessToken); 
                } else {
                    std::cout << "Invalid option selected.\n";
                }
                break;
            }
            case 6: { 
                if (wsThread.joinable()) {
                    std::cout << "WebSocket server is already running.\n";
                } else {
                    wsThread = std::thread([&wsServer]() {
                        wsServer.startServer(); 
                    });
                    std::cout << "WebSocket server started on port 9002.\n";
                }
                break;
            }

            case 7: { 
                static WebSocketClient wsClient;

                if (!wsThread.joinable()) {
                    std::cerr << "WebSocket server is not running. Please start it first.\n";
                    break;
                }

                if (!wsClient.isConnected()) {
                    wsClient.connect("ws://localhost:9002");
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                std::string symbol;
                std::cout << "Enter symbol to subscribe (e.g., BTC-PERPETUAL, ETH-PERPETUAL): ";
                std::getline(std::cin >> std::ws, symbol);

                if (!symbol.empty()) {
                    wsClient.send("subscribe:" + symbol);
                    std::cout << "Subscribing to symbol: " << symbol << "\n";
                } else {
                    std::cerr << "Symbol cannot be empty.\n";
                }

                break;
            }

            case 0:
                std::cout << "Exiting the program.\n";
                if (wsThread.joinable()) {
                    wsThread.join();
                }
                return 0;
            default:
                std::cout << "Invalid choice. Please try again.\n";
        }
    }
    if (wsThread.joinable()) {
        wsThread.join();
    }

    return 0;
}
