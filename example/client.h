#ifndef CLIENT_H_
#define CLIENT_H_

#include <string>
#include <string_view>

#include "../src/message_handler.h"
#include "../src/util/tasker.h"

class Client : public MessageHandler<Client> {
public:
    Client(std::string_view id);
    ~Client();

    void Send(std::string_view dst, std::string_view chat);

private:
    friend MessageHandler;

    void Post(Message message);
    void OnMessage(Message message);

    std::string id_;

    // Need to release before other member variable
    Tasker<Message> tasker_;
};

#endif // CLIENT_H_