#ifndef MESSAGE_HANDLER_H_
#define MESSAGE_HANDLER_H_

#include <memory>
#include <utility>

#include "message.h"

class MessageHandler {
public:
    template <typename T>
    MessageHandler(T&& handler)
        : pimpl_ { std::make_unique<HandlerModel<T>>(std::forward<T>(handler)) }
    {
    }

    void Post(Message&& message)
    {
        pimpl_->Post(std::move(message));
    }

private:
    struct HandlerConcept {
        virtual ~HandlerConcept() = default;
        virtual void Post(Message&& message) = 0;
    };

    template <typename T>
    struct HandlerModel : public HandlerConcept {
        template <typename U>
        HandlerModel(U&& handler)
            : object { std::forward<U>(handler) }
        {
        }

        void Post(Message&& message) override
        {
            object.Post(std::move(message));
        }

        T object;
    };

    std::unique_ptr<HandlerConcept> pimpl_;
};

#endif // MESSAGE_HANDLER_H_