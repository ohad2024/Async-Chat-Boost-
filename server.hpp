#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include "ChatMessage.hpp"  

struct ClientInfo {
    std::string _username;
    std::shared_ptr<boost::asio::ip::tcp::socket> _socket;
};


class ChatServer {
    boost::asio::io_context _ioContext;
    boost::asio::ip::tcp::acceptor _acceptor;
    std::vector<ClientInfo> _clients; 
    
    static constexpr int serverPort = 54000; 
    static constexpr int bufferSize = 1024;

    void acceptClients();
    void asyncAccept();

    void receiveUsername(std::shared_ptr<boost::asio::ip::tcp::socket> clientSocket);

    void asyncRead(std::shared_ptr<boost::asio::ip::tcp::socket> clientSocket);
    void asyncWrite(std::shared_ptr<boost::asio::ip::tcp::socket> clientSocket, const std::string& message);
    void handleMessage(const ChatMessage& message, std::shared_ptr<boost::asio::ip::tcp::socket> senderSocket);

    void broadcastMessage(const ChatMessage& message, std::shared_ptr<boost::asio::ip::tcp::socket> senderSocket = nullptr);
    void processRecipients(const ChatMessage& message);
    void removeClient(std::shared_ptr<boost::asio::ip::tcp::socket> clientSocket);

public:
    ChatServer();
    void runServer();
};
