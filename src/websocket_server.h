#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <unordered_map>
#include <string>
#include <vector>

class WebSocketServer {
public:
    WebSocketServer();
    void startServer(); 

private:
    using server = websocketpp::server<websocketpp::config::asio>;
    server m_server;
    std::unordered_map<std::string, std::vector<websocketpp::connection_hdl>> m_subscribers;

    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);
    void subscribeSymbol(const std::string& symbol, websocketpp::connection_hdl hdl);
    void sendOrderBookUpdate(const std::string& symbol);
};

#endif 
