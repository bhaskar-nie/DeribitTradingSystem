#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <iostream>
#include <string>
#include <thread>

typedef websocketpp::client<websocketpp::config::asio_client> client;

class WebSocketClient {
public:
    WebSocketClient() {
        m_client.init_asio();
        m_client.set_open_handler([&](websocketpp::connection_hdl hdl) {
            std::cout << "Connected to server." << std::endl;
            m_hdl = hdl;
        });

        m_client.set_message_handler([&](websocketpp::connection_hdl, client::message_ptr msg) {
            std::cout << "Received from server: " << msg->get_payload() << std::endl;
        });

        m_client.set_fail_handler([&](websocketpp::connection_hdl) {
            std::cerr << "Connection failed." << std::endl;
        });

        m_client.set_close_handler([&](websocketpp::connection_hdl) {
            std::cout << "Connection closed by server." << std::endl;
        });
    }

    void connect(const std::string& uri) {
        websocketpp::lib::error_code ec;
        client::connection_ptr con = m_client.get_connection(uri, ec);
        if (ec) {
            std::cerr << "Could not create connection because: " << ec.message() << std::endl;
            return;
        }

        m_client.connect(con);
        m_client_thread = std::thread([this]() { m_client.run(); });
    }

    void send(const std::string& message) {
        if (m_hdl.lock()) { 
            websocketpp::lib::error_code ec;
            m_client.send(m_hdl, message, websocketpp::frame::opcode::text, ec);
            if (ec) {
                std::cerr << "Send failed: " << ec.message() << std::endl;
            }
        } else {
            std::cerr << "Send failed: Not connected to server." << std::endl;
        }
    }

    bool isConnected() const {
        return !m_hdl.expired(); 
    }

    ~WebSocketClient() {
        m_client.stop();
        if (m_client_thread.joinable()) {
            m_client_thread.join();
        }
    }

private:
    client m_client;
    websocketpp::connection_hdl m_hdl;
    std::thread m_client_thread;
};
