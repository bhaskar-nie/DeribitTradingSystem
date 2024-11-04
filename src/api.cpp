#include <string>
#include<bits/stdc++.h>
#include <curl/curl.h>
#include <iostream>
#include "api.h"
#include <vector>
#include <json/json.h>
#include <mutex>
using namespace Json;
using json = nlohmann::json;
using namespace std;

InstrumentManager instrumentManager;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    size_t totalSize = size * nmemb;
    out->append(static_cast<const char*>(contents), totalSize);
    return totalSize;
}

std::string getAccessToken(const std::string& clientId, const std::string& clientSecret) {
    CURL* curl;
    std::string readBuffer;
    
    curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return ""; 
    }

    std::string encodedClientId = curl_easy_escape(curl, clientId.c_str(), clientId.length());
    std::string encodedClientSecret = curl_easy_escape(curl, clientSecret.c_str(), clientSecret.length());

    std::string url = "https://test.deribit.com/api/v2/public/auth?client_id=" + encodedClientId +
                      "&client_secret=" + encodedClientSecret + "&grant_type=client_credentials";

    struct curl_slist* headers = curl_slist_append(nullptr, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        std::cerr << "CURL request failed: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
    }

    try {
        json responseJson = json::parse(readBuffer);
        if (responseJson.contains("result") && responseJson["result"].contains("access_token")) {
            std::string accessToken = responseJson["result"]["access_token"];
            std::cout << "Access Token: " << accessToken << std::endl;
            curl_easy_cleanup(curl); 
            return accessToken; 
        } else {
            std::cerr << "Failed to get access token: " << responseJson.dump() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }

    curl_easy_cleanup(curl);
    return ""; 
}

bool handleResponse(const std::string& response) {
    Json::Reader reader;
    Json::Value jsonResponse;
    if (reader.parse(response, jsonResponse)) {
        if (jsonResponse.isMember("error")) {
            std::cerr << "Error: " << jsonResponse["error"]["message"].asString() << std::endl;
            return false;
        } else {
            std::cout << "Success: All orders of specified kind or type have been cancelled." << std::endl;
            return true;
        }
    } else {
        std::cerr << "Failed to parse response." << std::endl;
        return false;
    }
}

std::string urlEncode(const std::string &value) {
    CURL *curl = curl_easy_init();
    char *output = curl_easy_escape(curl, value.c_str(), value.length());
    std::string encoded(output);
    curl_free(output);
    curl_easy_cleanup(curl);
    return encoded;
}

void fetchInstruments(const std::string& currency, const std::string& kind, const std::string& accessToken, InstrumentManager& instrumentManager) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    std::string url = "https://test.deribit.com/api/v2/public/get_instruments?currency=" + currency + "&kind=" + kind;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            try {
                json responseJson = json::parse(readBuffer);
                if (responseJson.contains("result")) {
                    std::cout << "Available " << kind << " instruments for " << currency << ":\n";
                    for (const auto& instrument : responseJson["result"]) {
                        std::string instrumentName = instrument["instrument_name"];
                        instrumentManager.addInstrument(instrumentName);
                        
                        std::cout << " - Instrument Name: " << instrumentName << "\n";
                        std::cout << "   Type: " << instrument["kind"] << "\n";
                        std::cout << "   Taker Commission: " << instrument["taker_commission"] << "\n";
                        std::cout << "   Tick Size: " << instrument["tick_size"] << "\n";
                        std::cout << "   Settlement Period: " << instrument["settlement_period"] << "\n";
                        std::cout << "   Is Active: " << (instrument["is_active"] ? "Yes" : "No") << "\n";
                        std::cout << "--------------------------------------\n";
                    }
                } else {
                    std::cerr << "No instruments found for " << currency << " " << kind << ".\n";
                }
            } catch (const std::exception& e) {
                std::cerr << "Error parsing response: " << e.what() << std::endl;
            }
        }

        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Failed to initialize CURL." << std::endl;
    }
}


void displayBuyOrderResponse(const json& responseJson) {
    if (responseJson.contains("result") && responseJson["result"].contains("order")) {
        const auto& order = responseJson["result"]["order"];

        std::cout << "Buy Order Details:\n";
        std::cout << "  Order ID: " << order["order_id"] << "\n";
        std::cout << "  Status: " << order["order_state"] << "\n";
        std::cout << "  Instrument: " << order["instrument_name"] << "\n";
        std::cout << "  Direction: " << order["direction"] << "\n";
        std::cout << "  Amount: " << order["amount"] << "\n";
        std::cout << "  Filled Amount: " << order["filled_amount"] << "\n";
        std::cout << "  Average Price: " << order["average_price"] << "\n";
        std::cout << "  Timestamp: " << order["creation_timestamp"] << "\n"; 
    } else {
        std::cerr << "Failed to retrieve buy order details.\n";
        if (responseJson.contains("error")) {
            std::cerr << "Error: " << responseJson["error"]["message"] << "\n";
        }
    }
}


void placeBuyOrder(const std::string& currency, const std::string& kind, const std::string& symbol, double amount, const std::string& label, const std::string& type, const std::string& accessToken) {
    if (amount <= 0) {
        std::cerr << "Invalid amount: must be greater than 0.\n";
        return;
    }

    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    json requestData = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "private/buy"},
        {"params", {
            {"amount", amount},
            {"instrument_name", symbol},
            {"type", type},
            {"currency", currency},
            {"kind", kind}
        }}
    };

    //std::cout << "Request Data: " << requestData.dump(4) << std::endl; 

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://test.deribit.com/api/v2/private/buy");
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestData.dump().c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            try {
                json responseJson = json::parse(readBuffer);
                //std::cout << "Full Buy Order Response: " << responseJson.dump(4) << std::endl; 
                displayBuyOrderResponse(responseJson);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing response: " << e.what() << std::endl;
            }
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    } else {
        std::cerr << "Failed to initialize CURL." << std::endl;
    }
}


void displaySellOrderResponse(const json& responseJson) {
    if (responseJson.contains("result") && responseJson["result"].contains("order")) {
        const auto& order = responseJson["result"]["order"];

        std::cout << "Sell Order Details:\n";
        std::cout << "  Order ID: " << order["order_id"] << "\n";
        std::cout << "  Status: " << order["order_state"] << "\n";
        std::cout << "  Instrument: " << order["instrument_name"] << "\n";
        std::cout << "  Direction: " << order["direction"] << "\n";
        std::cout << "  Amount: " << order["amount"] << "\n";
        std::cout << "  Price: " << order["price"] << "\n";
        std::cout << "  Trigger Price: " << order["trigger_price"] << "\n";
        std::cout << "  Timestamp: " << order["creation_timestamp"] << "\n"; 
    } else {
        std::cerr << "Failed to retrieve sell order details.\n";
        if (responseJson.contains("error")) {
            std::cerr << "Error: " << responseJson["error"]["message"] << "\n";
        }
    }
}


void placeSellOrder(const std::string& currency, const std::string& kind, const std::string& symbol, double amount, double price, const std::string& trigger, double triggerPrice, const std::string& type, const std::string& accessToken) {
    if (amount <= 0) {
        std::cerr << "Invalid amount: must be greater than 0.\n";
        return;
    }
    
    if (price <= 0) {
        std::cerr << "Invalid price: must be greater than 0.\n";
        return;
    }

    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    json requestData = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "private/sell"},
        {"params", {
            {"amount", amount},
            {"instrument_name", symbol},
            {"price", price},
            {"trigger", trigger},
            {"trigger_price", triggerPrice},
            {"type", type},
            {"currency", currency},
            {"kind", kind}
        }}
    };

    //std::cout << "Request Data: " << requestData.dump(4) << std::endl; 

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://test.deribit.com/api/v2/private/sell");
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestData.dump().c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            try {
                json responseJson = json::parse(readBuffer);
                //std::cout << "Full Sell Order Response: " << responseJson.dump(4) << std::endl; 
                displaySellOrderResponse(responseJson);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing response: " << e.what() << std::endl;
            }
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    } else {
        std::cerr << "Failed to initialize CURL." << std::endl;
    }
}


void sendGetRequest(const std::string& url, const std::string& accessToken) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); 
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            try {
                json responseJson = json::parse(readBuffer);
                std::cout << "Response:\n" << responseJson.dump(4) << std::endl; 

                if (responseJson.contains("error")) {
                    std::cerr << "Error: " << responseJson["error"]["message"] << "\n";
                }
            } catch (const std::exception& e) {
                std::cerr << "Error parsing response: " << e.what() << std::endl;
            }
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    } else {
        std::cerr << "Failed to initialize CURL." << std::endl;
    }
}

void sendGetRequest(const std::string& url, const std::string& accessToken, std::string& response) {
    CURL* curl;
    CURLcode res;
    response.clear(); 

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);  
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    } else {
        std::cerr << "CURL initialization failed." << std::endl;
    }
}

void cancelOrder(const std::string& orderId, const std::string& accessToken) {
    std::string url = "https://test.deribit.com/api/v2/private/cancel?order_id=" + orderId;
    std::string response; 
    sendGetRequest(url, accessToken, response);
    handleResponse(response); 
}

void cancelAllOrders(const std::string& accessToken) {
    std::string url = "https://test.deribit.com/api/v2/private/cancel_all";
    std::string response; 
    sendGetRequest(url, accessToken, response); 
    handleResponse(response); 
}

void cancelAllByCurrency(const std::string& currency, const std::string& kind, const std::string& accessToken) {
    std::string url = "https://test.deribit.com/api/v2/private/cancel_all_by_currency?currency=" + currency + "&kind=" + kind;
    std::string response; 
    sendGetRequest(url, accessToken, response); 
    handleResponse(response); 
}

void cancelAllByCurrencyPair(const std::string& currencyPair, const std::string& kind, const std::string& accessToken) {
    std::string url = "https://test.deribit.com/api/v2/private/cancel_all_by_currency_pair?currency_pair=" + currencyPair + "&kind=" + kind;
    std::string response; 
    sendGetRequest(url, accessToken, response); 
    handleResponse(response); 
}

void cancelAllByInstrument(const std::string& instrumentName, const std::string& type, const std::string& accessToken) {
    std::string url = "https://test.deribit.com/api/v2/private/cancel_all_by_instrument?instrument_name=" + instrumentName + "&type=" + type;
    std::string response; 
    sendGetRequest(url, accessToken, response); 
    handleResponse(response); 
}

void cancelAllByKindOrType(const std::vector<std::string>& currencies, const std::string& kind, const std::string& accessToken) {
    std::string url = "https://test.deribit.com/api/v2/private/cancel_all_by_kind_or_type?currency=";
    for (size_t i = 0; i < currencies.size(); ++i) {
        url += currencies[i];
        if (i < currencies.size() - 1) url += ",";
    }
    url += "&kind=" + kind;

    std::string response;
    sendGetRequest(url, accessToken, response);

    handleResponse(response);
}

void cancelByLabel(const std::string& label, const std::string& accessToken) {
    std::string url = "https://test.deribit.com/api/v2/private/cancel_by_label?label=" + label;
    std::string response; 
    sendGetRequest(url, accessToken, response); 
    handleResponse(response); 
}


std::string sendRequest(const std::string& url, const std::string& jsonData, const std::string& accessToken) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "Error in request: " << curl_easy_strerror(res) << std::endl;
            return {};
        }
    } else {
        std::cerr << "Failed to initialize CURL." << std::endl;
    }
    return readBuffer;
}


void editOrder(const std::string& orderId, double newAmount, double newPrice, const std::string& accessToken) {
    std::string url = "https://test.deribit.com/api/v2/private/edit";

    json payload = {
        {"jsonrpc", "2.0"},
        {"method", "private/edit"},
        {"params", {
            {"order_id", orderId},
            {"amount", newAmount},
            {"price", newPrice},
            {"advanced", "implv"} 
        }},
        {"id", 1}
    };

    std::cout << "Editing Order ID: " << orderId 
              << " with amount: " << newAmount 
              << " and price: " << newPrice << std::endl;

    std::string response = sendRequest(url, payload.dump(), accessToken);

    if (response.empty()) {
        std::cerr << "Received empty response from the API." << std::endl;
        return;
    }

    json jsonResponse;
    try {
        jsonResponse = json::parse(response);
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse JSON response: " << e.what() << std::endl;
        return;
    }

    if (jsonResponse.contains("error")) {
        std::cerr << "API Error: " << jsonResponse["error"]["message"] << std::endl;
    } else {
        const auto& order = jsonResponse["result"]["order"];
        std::cout << "Order edited successfully! New details:\n"
                  << "Order ID: " << order["order_id"] << "\n"
                  << "New Amount: " << order["amount"] << "\n"
                  << "New Price: " << order["price"] << "\n"
                  << "Order State: " << order["order_state"] << std::endl;
    }
}


void editOrderByLabel(const std::string& label, double amount, double price, const std::string& instrumentName, const std::string& accessToken) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    json requestData = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "private/edit_by_label"},
        {"params", {
            {"label", label},
            {"amount", amount},
            {"price", price},
            {"instrument_name", instrumentName}
        }}
    };

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://test.deribit.com/api/v2/private/edit_by_label");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestData.dump().c_str());

        struct curl_slist* headers = curl_slist_append(nullptr, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            json responseJson = json::parse(readBuffer);
            std::cout << "Edit Order by Label Response: " << responseJson.dump(4) << std::endl; 

            if (responseJson.contains("error")) {
                std::cerr << "API Error: " << responseJson["error"]["message"] << std::endl;
            }
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    } else {
        std::cerr << "Failed to initialize CURL." << std::endl;
    }
}


size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

std::string getOrderBook(const std::string& instrument_name) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    std::string url = "https://test.deribit.com/api/v2/public/get_order_book?instrument_name=" + instrument_name;

    try {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();

        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback); 
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }

        json jsonResponse = json::parse(readBuffer);

        if (jsonResponse.contains("error")) {
            std::cerr << "API Error: " << jsonResponse["error"]["message"] << std::endl;
            return ""; 
        }

        std::ostringstream oss;
        oss << "Order Book for " << instrument_name << ":\nBids:\n";
        for (const auto& bid : jsonResponse["result"]["bids"]) {
            oss << "Price: " << bid[0].get<double>() << ", Amount: " << bid[1].get<double>() << "\n";
        }

        oss << "\nAsks:\n";
        for (const auto& ask : jsonResponse["result"]["asks"]) {
            oss << "Price: " << ask[0].get<double>() << ", Amount: " << ask[1].get<double>() << "\n";

        }

        return oss.str(); 

    } catch (const json::parse_error& e) {
        std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
        return ""; 
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return ""; 
    }

    curl_global_cleanup();
    return ""; 
}

void getOrderBook2(const std::string& instrument_name) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    std::string url = "https://test.deribit.com/api/v2/public/get_order_book?instrument_name=" + instrument_name;

    try {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();

        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }

        Json::CharReaderBuilder readerBuilder;
        Json::Value jsonResponse;
        std::string errs;
        std::istringstream s(readBuffer); 

        if (Json::parseFromStream(readerBuilder, s, &jsonResponse, &errs)) {
            std::cout << "Order Book for " << instrument_name << ":\n";

            std::cout << "Bids:\n";
            for (const auto& bid : jsonResponse["result"]["bids"]) {
                std::cout << "Price: " << bid[0].asDouble() << ", Amount: " << bid[1].asDouble() << std::endl;
            }

            std::cout << "\nAsks:\n";
            for (const auto& ask : jsonResponse["result"]["asks"]) {
                std::cout << "Price: " << ask[0].asDouble() << ", Amount: " << ask[1].asDouble() << std::endl;
            }
        } else {
            std::cerr << "Failed to parse JSON: " << errs << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    int choice;
    std::cout << "\nWould you like to:\n";
    std::cout << "1. Get another Order Book\n";
    std::cout << "0. Go back to Main Menu\n";
    std::cin >> choice;

    if (choice == 1) {
        std::string new_instrument_name;
        std::cout << "Enter Instrument Name (e.g., BTC-PERPETUAL): ";
        std::cin >> new_instrument_name;
        getOrderBook(new_instrument_name); 
    } else {
        return;
    }

    curl_global_cleanup();
}


void getOrderBookByInstrumentId(int instrument_id) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    try {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (curl) {
            std::string url = "https://test.deribit.com/api/v2/public/get_order_book_by_instrument_id?instrument_id=" + std::to_string(instrument_id) + "&depth=1";
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            }

            curl_easy_cleanup(curl);
        }

        Json::Value jsonResponse;
        Json::CharReaderBuilder readerBuilder;
        std::string errs;
        std::istringstream s(readBuffer);

        if (Json::parseFromStream(readerBuilder, s, &jsonResponse, &errs)) {
            std::cout << "Order Book for Instrument ID " << instrument_id << ":\n";

            std::cout << "Bids:\n";
            for (const auto& bid : jsonResponse["result"]["bids"]) {
                std::cout << "Price: " << bid[0].asDouble() << ", Amount: " << bid[1].asDouble() << std::endl;
            }

            std::cout << "\nAsks:\n";
            for (const auto& ask : jsonResponse["result"]["asks"]) {
                std::cout << "Price: " << ask[0].asDouble() << ", Amount: " << ask[1].asDouble() << std::endl;
            }
        } else {
            std::cerr << "Failed to parse JSON: " << errs << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    curl_global_cleanup();
}

void getPositionByInstrumentName(const std::string& instrument_name, const std::string& access_token) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    try {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (curl) {
            std::string url = "https://test.deribit.com/api/v2/private/get_position?instrument_name=" + instrument_name;

            struct curl_slist *headers = nullptr;
            headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());
            headers = curl_slist_append(headers, "Content-Type: application/json");

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            }

            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
        }

        json jsonResponse = json::parse(readBuffer);

        if (jsonResponse.contains("error")) {
            std::cerr << "API Error: " << jsonResponse["error"]["message"] << std::endl;
            return;
        }

        const auto& result = jsonResponse["result"];
        std::cout << "Position for " << instrument_name << ":\n";
        std::cout << "Direction: " << result["direction"].get<std::string>() << "\n";
        std::cout << "Average Price: " << result["average_price"].get<double>() << "\n";
        std::cout << "Delta: " << result["delta"].get<double>() << "\n";
        std::cout << "Floating P/L: " << result["floating_profit_loss"].get<double>() << "\n";
        std::cout << "Leverage: " << result["leverage"].get<int>() << "\n";
        std::cout << "Size: " << result["size"].get<double>() << "\n";
        std::cout << "Mark Price: " << result["mark_price"].get<double>() << "\n";
        
    } catch (const json::parse_error& e) {
        std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    curl_global_cleanup();
}

void getAllPositions(const std::string& currency, const std::string& kind, const std::string& access_token) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    try {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (curl) {
            std::string url = "https://test.deribit.com/api/v2/private/get_positions?currency=" + currency + "&kind=" + kind;

            struct curl_slist *headers = nullptr;
            headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());
            headers = curl_slist_append(headers, "Content-Type: application/json");

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            
            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            }

            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
        }

        json jsonResponse = json::parse(readBuffer);

        if (jsonResponse.contains("error")) {
            std::cerr << "API Error: " << jsonResponse["error"]["message"] << std::endl;
            return;
        }

        std::cout << "Positions for " << currency << " (" << kind << "):\n";
        for (const auto& position : jsonResponse["result"]) {
            std::cout << "Instrument Name: " << position["instrument_name"].get<std::string>() << "\n";
            std::cout << "Direction: " << position["direction"].get<std::string>() << "\n";
            std::cout << "Average Price: " << position["average_price"].get<double>() << "\n";
            std::cout << "Delta: " << position["delta"].get<double>() << "\n";
            std::cout << "Floating P/L: " << position["floating_profit_loss"].get<double>() << "\n";
            std::cout << "Leverage: " << position["leverage"].get<int>() << "\n";
            std::cout << "Size: " << position["size"].get<double>() << "\n";
            std::cout << "Mark Price: " << position["mark_price"].get<double>() << "\n\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    curl_global_cleanup();
}