#pragma once

#include <boost/asio.hpp>
#include <string>
#include <vector>
#include "ChatMessage.hpp" 


class ChatClient {
    boost::asio::io_context _ioContext;
    boost::asio::ip::tcp::socket _clientSocket;
    std::string _userName; 
    bool _isRunning = true;

    static const int bufferSize;
    static const std::string serverIp;
    static const int serverPort;

    void getUserName();
    void asyncConnect();
    void asyncRead();
    void asyncWrite(const std::string& message);
    ChatMessage createChatMessage(const std::string& input);
    std::vector<std::string> parseRecipientUsernames(const std::string& input);
    std::string extractMessage(const std::string& input);
    void asyncSendUsername();

public:
    ChatClient();
    void runClient();
};
