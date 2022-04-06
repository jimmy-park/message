#ifndef MESSAGE_HANDLER_H_
#define MESSAGE_HANDLER_H_

#include "message.h"

struct AbstractMessageHandler {
    virtual ~AbstractMessageHandler() = default;
    virtual void Post(Message message) = 0;
};

template <typename Derived>
class MessageHandler : public AbstractMessageHandler {
protected:
    void Post(Message message) override { static_cast<Derived*>(this)->Post(std::move(message)); }
    void OnMessage(Message message) { static_cast<Derived*>(this)->OnMessage(std::move(message)); }
};

#endif // MESSAGE_HANDLER_H_