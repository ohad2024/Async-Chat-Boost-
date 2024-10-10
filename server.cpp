#include "server.hpp"
#include <iostream>

const int ChatServer::serverPort = 54000;
const int ChatServer::bufferSize = 1024;

ChatServer::ChatServer()
    : acceptor(ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), serverPort)) {}

void ChatServer::acceptClients() {
    asyncAccept();
    ioContext.run();
}

void ChatServer::asyncAccept() {
    auto clientSocket = std::make_shared<boost::asio::ip::tcp::socket>(ioContext);
    acceptor.async_accept(*clientSocket,
        [this, clientSocket](const boost::system::error_code& error) {
            if (!error) {
                receiveUsername(clientSocket); // Read username after connection
                asyncAccept(); // Continue accepting new connections
            }
        });
}

void ChatServer::receiveUsername(std::shared_ptr<boost::asio::ip::tcp::socket> clientSocket) {
    auto buffer = std::make_shared<std::vector<char>>(bufferSize);
    clientSocket->async_read_some(boost::asio::buffer(*buffer),
        [this, clientSocket, buffer](const boost::system::error_code& error, std::size_t length) {
            if (!error) {
                std::string username(buffer->data(), length);

                // Add client with username and socket
                ClientInfo newClient = { username, clientSocket };
                clients.push_back(newClient);

                // Notify all clients about the new user
                ChatMessage joinMsg;
                joinMsg.messageType = MessageType::BROADCAST;
                joinMsg.senderUserName = "Server";
                joinMsg.time = ChatMessage::getCurrentTime(); // Set the time for the message
                joinMsg.message = username + " has joined the chat.";
                broadcastMessage(joinMsg);

                // Start reading messages from the client
                asyncRead(clientSocket);
            }
        });
}

void ChatServer::asyncRead(std::shared_ptr<boost::asio::ip::tcp::socket> clientSocket) {
    auto buffer = std::make_shared<std::vector<char>>(bufferSize);
    clientSocket->async_read_some(boost::asio::buffer(*buffer),
        [this, clientSocket, buffer](const boost::system::error_code& error, std::size_t length) {
            if (!error) {
                std::string receivedData(buffer->data(), length);
                ChatMessage msg = ChatMessage::deserialize(receivedData);

                handleMessage(msg, clientSocket);
                asyncRead(clientSocket); // Continue reading from this client
            } else {
                removeClient(clientSocket);
            }
        });
}

void ChatServer::handleMessage(const ChatMessage& message, std::shared_ptr<boost::asio::ip::tcp::socket> senderSocket) {
    if (message.messageType == MessageType::BROADCAST) {
        broadcastMessage(message, senderSocket);
    } else {
        processRecipients(message);
    }
}

void ChatServer::broadcastMessage(const ChatMessage& message, std::shared_ptr<boost::asio::ip::tcp::socket> senderSocket) {
    for (const auto& client : clients) {
        if (senderSocket == nullptr || client.socket != senderSocket) { // Skip sender for regular broadcast
            asyncWrite(client.socket, message.serialize());
        }
    }
}

void ChatServer::processRecipients(const ChatMessage& message) {
    std::vector<std::string> uniqueRecipients; // Using std::string for recipient names
    for (const auto& recipientName : message.recipients) {
        if (std::find(uniqueRecipients.begin(), uniqueRecipients.end(), recipientName) == uniqueRecipients.end()) {
            uniqueRecipients.push_back(recipientName);
        }
    }

    for (const auto& client : clients) {
        if (std::find(uniqueRecipients.begin(), uniqueRecipients.end(), client.username) != uniqueRecipients.end()) {
            asyncWrite(client.socket, message.serialize());
        }
    }
}


void ChatServer::asyncWrite(std::shared_ptr<boost::asio::ip::tcp::socket> clientSocket, const std::string& message) {
    auto buffer = std::make_shared<std::string>(message);
    boost::asio::async_write(*clientSocket, boost::asio::buffer(*buffer),
        [buffer](const boost::system::error_code& error, std::size_t length) {
            if (error) {
                std::cerr << "Error sending message: " << error.message() << std::endl;
            }
        });
}

void ChatServer::removeClient(std::shared_ptr<boost::asio::ip::tcp::socket> clientSocket) {
    clients.erase(std::remove_if(clients.begin(), clients.end(),
        [clientSocket](const ClientInfo& client) {
            return client.socket == clientSocket;
        }), clients.end());
}

void ChatServer::runServer() {
    std::cout << "Server is working" << std::endl;
    acceptClients();
}
