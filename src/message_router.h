#ifndef MESSAGE_ROUTER_H_
#define MESSAGE_ROUTER_H_

#include <shared_mutex>
#include <string_view>
#include <unordered_map>

#include "message_handler.h"
#include "util/singleton.h"

class MessageRouter : public Singleton<MessageRouter> {
public:
    MessageRouter();

    void Register(std::string_view id, AbstractMessageHandler* handler);
    void Unregister(std::string_view id);

    template <typename... Args>
    void Post(Args&&... args)
    {
        tasker_.Post(std::forward<Args>(args)...);
    }

private:
    void OnMessage(Message message);

    Tasker<Message> tasker_;
    std::unordered_map<std::string_view, AbstractMessageHandler*> handlers_;
    std::shared_mutex mutex_;
};

#endif // MESSAGE_ROUTER_H_