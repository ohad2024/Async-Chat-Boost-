#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <utility> // For std::pair
#include "ChatMessage.hpp"  // Include the ChatMessage definition

/**
 * @brief Structure to represent a connected client with username and socket.
 */
struct ClientInfo {
    std::string username;
    std::shared_ptr<boost::asio::ip::tcp::socket> socket;
};


class ChatServer {
    boost::asio::io_context ioContext; ///< IO context for handling asynchronous operations
    boost::asio::ip::tcp::acceptor acceptor; ///< TCP acceptor for handling incoming connections
    std::vector<ClientInfo> clients; ///< List of connected clients

    static const int serverPort;
    static const int bufferSize;

    void acceptClients();
    void asyncAccept();

    /**
     * @brief Asynchronously reads the username from a newly connected client.
     * @param clientSocket The socket of the client who just connected.
     */
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
