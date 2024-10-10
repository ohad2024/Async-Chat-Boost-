#pragma once

#include <boost/asio.hpp>
#include <string>
#include <vector>
#include "ChatMessage.hpp"  // Include the ChatMessage definition


class ChatClient {
    boost::asio::io_context ioContext; ///< IO context for handling asynchronous operations
    boost::asio::ip::tcp::socket clientSocket; ///< Socket for connecting to the server
    std::string userName; ///< The client's username
    bool isRunning = true; ///< Indicates if the client is still running
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
