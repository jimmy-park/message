#include "client.h"

#include <iostream>

#include "../src/message_router.h"

Client::Client(std::string_view id)
    : id_ { id }
{
    MessageRouter::GetInstance().Register(id_, this);
}

Client::~Client()
{
    MessageRouter::GetInstance().Unregister(id_);
}

void Client::Send(std::string_view dst, std::string_view chat)
{
    Message msg;
    msg.from = id_;
    msg.to = dst;
    msg << chat;

    MessageRouter::GetInstance().Post(std::move(msg));
}

void Client::OnMessage(Message message)
{
    std::string chat;

    message >> chat;

    std::cout << message.from << " -> " << message.to << " : " << chat << '\n';
}