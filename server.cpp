#include "server.hpp"
#include <iostream>

const int ChatServer::serverPort = 54000;
const int ChatServer::bufferSize = 1024;


ChatServer::ChatServer()
    : _acceptor(_ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), serverPort)) {}

void ChatServer::acceptClients() {
    asyncAccept();
    _ioContext.run();
}

void ChatServer::asyncAccept() {
    auto clientSocket = std::make_shared<boost::asio::ip::tcp::socket>(_ioContext);
    _acceptor.async_accept(*clientSocket,
        [this, clientSocket](const boost::system::error_code& error) {
            if (!error) {
                receiveUsername(clientSocket); 
                asyncAccept();
            }
        });
}

void ChatServer::receiveUsername(std::shared_ptr<boost::asio::ip::tcp::socket> clientSocket) {
    auto buffer = std::make_shared<std::vector<char>>(bufferSize);
    clientSocket->async_read_some(boost::asio::buffer(*buffer),
        [this, clientSocket, buffer](const boost::system::error_code& error, std::size_t length) {
            if (!error) {
                std::string username(buffer->data(), length);

                ClientInfo newClient = { username, clientSocket };
                _clients.push_back(newClient);

                ChatMessage joinMsg;
                joinMsg.messageType = MessageType::BROADCAST;
                joinMsg.senderUserName = "Server";
                joinMsg.time = ChatMessage::getCurrentTime(); 
                joinMsg.message = username + " has joined the chat.";
                broadcastMessage(joinMsg);

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
                asyncRead(clientSocket);
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
    for (const auto& client : _clients) {
        if (senderSocket == nullptr || client.socket != senderSocket) { 
            asyncWrite(client.socket, message.serialize());
        }
    }
}

void ChatServer::processRecipients(const ChatMessage& message) {
    std::vector<std::string> uniqueRecipients;
    for (const auto& recipientName : message.recipients) {
        if (std::find(uniqueRecipients.begin(), uniqueRecipients.end(), recipientName) == uniqueRecipients.end()) {
            uniqueRecipients.push_back(recipientName);
        }
    }

    for (const auto& client : _clients) {
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
    _clients.erase(std::remove_if(_clients.begin(), _clients.end(),
        [clientSocket](const ClientInfo& client) {
            return client.socket == clientSocket;
        }), _clients.end());
}

void ChatServer::runServer() {
    std::cout << "Server is working" << std::endl;
    acceptClients();
}
