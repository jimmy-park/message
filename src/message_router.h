#ifndef MESSAGE_ROUTER_H_
#define MESSAGE_ROUTER_H_

#include <shared_mutex>
#include <string_view>
#include <unordered_map>

#include "singleton.h"
#include "tasker.h"

#include "message_handler.h"

class MessageRouter : public Singleton<MessageRouter>, public TaskerBase<MessageRouter, Message> {
public:
    ~MessageRouter();

    void Register(std::string_view id, MessageHandler handler);
    void Unregister(std::string_view id);

private:
    friend TaskerBase;

    void Process(Message message);

    std::unordered_map<std::string_view, MessageHandler> handlers_;
    std::shared_mutex mutex_;
};

#endif // MESSAGE_ROUTER_H_