#include "client.hpp"
#include <iostream>

const std::string ChatClient::serverIp = "127.0.0.1";
const int ChatClient::serverPort = 54000;
const int ChatClient::bufferSize = 1024;

ChatClient::ChatClient()
    : clientSocket(ioContext) {}

void ChatClient::getUserName() {
    std::cout << "Enter your username: ";
    std::getline(std::cin, userName);
    while (userName.empty()) {
        std::cout << "Username cannot be empty. Please enter your username: ";
        std::getline(std::cin, userName);
    }
}

void ChatClient::asyncConnect() {
    boost::asio::ip::tcp::resolver resolver(ioContext);
    auto endpoints = resolver.resolve(serverIp, std::to_string(serverPort));
    boost::asio::async_connect(clientSocket, endpoints,
        [this](const boost::system::error_code& error, const auto&) {
            if (!error) {
                asyncSendUsername();
                asyncRead();
            }
        });
}

void ChatClient::asyncSendUsername() {
    auto buffer = std::make_shared<std::string>(userName);
    boost::asio::async_write(clientSocket, boost::asio::buffer(*buffer),
        [buffer](const boost::system::error_code& error, std::size_t) {
            if (error) {
                std::cerr << "Error sending username: " << error.message() << std::endl;
            }
        });
}

void ChatClient::asyncRead() {
    auto buffer = std::make_shared<std::vector<char>>(bufferSize);
    clientSocket.async_read_some(boost::asio::buffer(*buffer),
        [this, buffer](const boost::system::error_code& error, std::size_t length) {
            if (!error) {
                std::string receivedData(buffer->data(), length);

                // Deserialize the received data into a ChatMessage
                ChatMessage msg = ChatMessage::deserialize(receivedData);
                std::cout << "[" << msg.time << "] " << msg.senderUserName << ": " << msg.message << std::endl;

                asyncRead(); // Continue reading next message from server
            }
        });
}

void ChatClient::asyncWrite(const std::string& message) {
    auto buffer = std::make_shared<std::string>(message);
    boost::asio::async_write(clientSocket, boost::asio::buffer(*buffer),
        [buffer](const boost::system::error_code& error, std::size_t) {
            if (error) {
                std::cerr << "Error sending message: " << error.message() << std::endl;
            }
        });
}

ChatMessage ChatClient::createChatMessage(const std::string& input) {
    ChatMessage msg;
    msg.senderUserName = userName;
    msg.time = ChatMessage::getCurrentTime();

    if (input[0] == '@') {
        msg.messageType = MessageType::MULTICAST;
        msg.recipients = parseRecipientUsernames(input);
        msg.message = extractMessage(input);
    } else {
        msg.messageType = MessageType::BROADCAST;
        msg.message = input;
    }

    return msg;
}

std::vector<std::string> ChatClient::parseRecipientUsernames(const std::string& input) {
    std::vector<std::string> recipients;
    size_t pos = 0;
    while ((pos = input.find("@", pos)) != std::string::npos) {
        size_t end = input.find(" ", pos);
        std::string username = input.substr(pos + 1, end - pos - 1);

        if (std::find(recipients.begin(), recipients.end(), username) == recipients.end()) {
            recipients.push_back(username);
        }

        pos = end;
    }
    return recipients;
}

std::string ChatClient::extractMessage(const std::string& input) {
    size_t pos = input.find_last_of("@");
    return input.substr(input.find(" ", pos + 1) + 1);
}

void ChatClient::runClient() {
    getUserName();
    asyncConnect();

    std::thread contextThread([this]() { ioContext.run(); });

    std::string messageText;
    while (std::getline(std::cin, messageText) && isRunning) {
        if (messageText == "exit") {
            isRunning = false;
            break;
        }

        ChatMessage msg = createChatMessage(messageText);
        std::string serializedMessage = msg.serialize();
        asyncWrite(serializedMessage);
    }

    clientSocket.close();
    contextThread.join();
}
