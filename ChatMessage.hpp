#pragma once
#include <string>
#include <vector>
#include <ctime>


enum class MessageType { BROADCAST, MULTICAST, UNICAST };


struct ChatMessage {
    std::string senderUserName; ///< Username of the sender
    std::string time;           ///< Time when the message was sent
    std::string message;        ///< Message content
    MessageType messageType;    ///< Type of the message: BROADCAST, MULTICAST, or UNICAST
    std::vector<std::string> recipients; ///< List of recipient usernames for MULTICAST and UNICAST


    std::string serialize() const {
        std::string serialized = senderUserName + "|" + time + "|" + std::to_string(static_cast<int>(messageType)) + "|";

        // Serialize recipients, separated by ";"
        for (const auto& recipient : recipients) {
            serialized += recipient + ";";
        }

        serialized += "|" + message; // Add message content
        return serialized;
    }

    static ChatMessage deserialize(const std::string& data) {
        ChatMessage msg;
        size_t pos1 = data.find('|');
        size_t pos2 = data.find('|', pos1 + 1);
        size_t pos3 = data.find('|', pos2 + 1);
        size_t pos4 = data.find('|', pos3 + 1);

        msg.senderUserName = data.substr(0, pos1);
        msg.time = data.substr(pos1 + 1, pos2 - pos1 - 1);
        msg.messageType = static_cast<MessageType>(std::stoi(data.substr(pos2 + 1, pos3 - pos2 - 1)));

        // Deserialize recipients
        size_t start = pos3 + 1;
        size_t pos;
        while ((pos = data.find(';', start)) < pos4) {
            msg.recipients.push_back(data.substr(start, pos - start));
            start = pos + 1;
        }

        msg.message = data.substr(pos4 + 1); // Remaining part is the message
        return msg;
    }

    static std::string getCurrentTime() {
        std::time_t now = std::time(nullptr);
        std::tm* localTime = std::localtime(&now);

        int hour = localTime->tm_hour;
        int minute = localTime->tm_min;

        std::string hourStr = (hour < 10 ? "0" : "") + std::to_string(hour);
        std::string minuteStr = (minute < 10 ? "0" : "") + std::to_string(minute);

        return hourStr + ":" + minuteStr;
    }
};
