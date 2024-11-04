#include <asio.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "websocket_server.h"
#include "api.h" 
#include <iostream>
#include <thread>
#include <chrono>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

WebSocketServer::WebSocketServer() {
    m_server.set_message_handler(bind(&WebSocketServer::on_message, this, _1, _2));
}

void WebSocketServer::startServer() {
    m_server.init_asio();
    m_server.set_reuse_addr(true);
    m_server.listen(9002);
    m_server.start_accept();
    std::cout << "WebSocket server started on port 9002" << std::endl;

    m_server.run(); 
}


void WebSocketServer::on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
    std::string payload = msg->get_payload();
    std::cout << "Received message: " << payload << std::endl;

    if (payload.find("subscribe:") == 0) {
        std::string symbol = payload.substr(10);
        subscribeSymbol(symbol, hdl);
    }
}

void WebSocketServer::subscribeSymbol(const std::string& symbol, websocketpp::connection_hdl hdl) {
    m_subscribers[symbol].push_back(hdl);
    std::cout << "Subscribed to symbol: " << symbol << std::endl;

    std::thread([this, symbol]() {
        while (true) {
            sendOrderBookUpdate(symbol);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }).detach();
}

void WebSocketServer::sendOrderBookUpdate(const std::string& symbol) {
    std::string orderBookData = getOrderBook(symbol);

    if (orderBookData.empty()) {
        std::cerr << "Failed to retrieve order book data for " << symbol << std::endl;
        return; 
    }

    for (auto& hdl : m_subscribers[symbol]) {
        try {
            m_server.send(hdl, orderBookData, websocketpp::frame::opcode::text);
        } catch (const websocketpp::exception& e) {
            std::cerr << "Error sending message: " << e.what() << std::endl;
        }
    }
}
