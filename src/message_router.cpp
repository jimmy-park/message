#include "message_router.h"

MessageRouter::MessageRouter()
    : tasker_ { [this](auto message) { OnMessage(std::move(message)); } }
{
}

void MessageRouter::Register(std::string_view id, AbstractMessageHandler* handler)
{
    std::lock_guard lock { mutex_ };

    handlers_.try_emplace(id, handler);
}

void MessageRouter::Unregister(std::string_view id)
{
    std::lock_guard lock { mutex_ };

    handlers_.erase(id);
}

void MessageRouter::OnMessage(Message message)
{
    std::shared_lock lock { mutex_ };

    if (auto it = handlers_.find(message.to); it != std::cend(handlers_))
        it->second->Post(std::move(message));
}