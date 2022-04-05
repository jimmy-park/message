#ifndef MESSAGE_HANDLER_H_
#define MESSAGE_HANDLER_H_

#include "message.h"
#include "util/tasker.h"

struct AbstractMessageHandler {
    virtual ~AbstractMessageHandler() = default;
    virtual void Post(Message message) = 0;
};

template <typename Derived>
class MessageHandler : public AbstractMessageHandler {
public:
    void Post(Message message) override { lopper_.Post(std::move(message)); }

private:
    Looper<Message> lopper_ { [this](auto message) { static_cast<Derived*>(this)->OnMessage(std::move(message)); } };
};

#endif // MESSAGE_HANDLER_H_