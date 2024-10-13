#include "client.hpp"
#include <iostream>

ChatClient::ChatClient()
    : _clientSocket(_ioContext) {}

void ChatClient::getUserName() {
    std::cout << "Enter your username: ";
    std::getline(std::cin, _userName);
    while (_userName.empty()) {
        std::cout << "Username cannot be empty. Please enter your username: ";
        std::getline(std::cin, _userName);
    }
}

void ChatClient::asyncConnect() {
    boost::asio::ip::tcp::resolver resolver(_ioContext);
    auto endpoints = resolver.resolve(serverIp, std::to_string(serverPort));
    boost::asio::async_connect(_clientSocket, endpoints,
        [this](const boost::system::error_code& error, const auto&) {
            if (!error) {
                asyncSendUsername();
                asyncRead();
            }
        });
}

void ChatClient::asyncSendUsername() {
    auto buffer = std::make_shared<std::string>(_userName);
    boost::asio::async_write(_clientSocket, boost::asio::buffer(*buffer),
        [buffer](const boost::system::error_code& error, std::size_t) {
            if (error) {
                std::cerr << "Error sending username: " << error.message() << std::endl;
            }
        });
}

void ChatClient::asyncRead() {
    auto buffer = std::make_shared<std::vector<char>>(bufferSize);
    _clientSocket.async_read_some(boost::asio::buffer(*buffer),
        [this, buffer](const boost::system::error_code& error, std::size_t length) {
            if (!error) {
                std::string receivedData(buffer->data(), length);

                ChatMessage msg = ChatMessage::deserialize(receivedData);
                std::cout << "[" << msg._time << "] " << msg._senderUserName << ": " << msg._message << std::endl;

                asyncRead();
            }
        });
}

void ChatClient::asyncWrite(const std::string& message) {
    auto buffer = std::make_shared<std::string>(message);
    boost::asio::async_write(_clientSocket, boost::asio::buffer(*buffer),
        [buffer](const boost::system::error_code& error, std::size_t) {
            if (error) {
                std::cerr << "Error sending message: " << error.message() << std::endl;
            }
        });
}

ChatMessage ChatClient::createChatMessage(const std::string& input) {
    ChatMessage msg;
    msg._senderUserName = _userName;
    msg._time = ChatMessage::getCurrentTime();

    if (input[0] == '@') {
        msg._messageType = MessageType::MULTICAST;
        msg._recipients = parseRecipientUsernames(input);
        msg._message = extractMessage(input);
    } else {
        msg._messageType = MessageType::BROADCAST;
        msg._message = input;
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

    std::thread contextThread([this]() { _ioContext.run(); });

    std::string messageText;
    while (std::getline(std::cin, messageText) && _isRunning) {
        if (messageText == "exit") {
            _isRunning = false;
            break;
        }

        ChatMessage msg = createChatMessage(messageText);
        std::string serializedMessage = msg.serialize();
        asyncWrite(serializedMessage);
    }

    _clientSocket.close();
    contextThread.join();
}
