#pragma once

#include "../state.hpp"
#include "reaction.hpp"
#include "overloaded.hpp"

class RunningGame : public GameState {
    struct WaitingForStub {

    };
    
    struct StubAccepted {
        Reaction reaction;
        std::chrono::system_clock::duration since_last_stub;
        std::string s1;
        std::string s2;

        StubAccepted(std::string&& s1, std::string&& s2)
            : reaction(),
              since_last_stub(std::chrono::system_clock::duration::zero()),
              s1(std::move(s1)),
              s2(std::move(s2)) {
        }
    };

    std::variant<WaitingForStub, StubAccepted> state;
public:
    RunningGame()
        : state(WaitingForStub()) {
    }

    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed,
                                                              InputState& input_state) {

        std::visit(overloaded{[&](WaitingForStub& waiting_for_stub) {
        }, [&](StubAccepted& stub_accepted) {
            stub_accepted.reaction.react_once([&]() {
                std::cout << "s1: " << stub_accepted.s1 << std::endl << "s2: " << stub_accepted.s2 << std::endl;
            });
            stub_accepted.since_last_stub += elapsed;
        }}, state);

        return {};
    }

    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) {
        std::visit(overloaded{[&](WaitingForStub& waiting_for_stub) {
            for(Message message : read_message_queue) {
                switch(message.type()) {
                    case MessageType::StubMessage: {
                        std::string s1;
                        std::string s2;
                        message >> s2 >> s1;
                        state = StubAccepted(std::move(s1), std::move(s2));
                        break;
                    }
                }
            }
        }, [&](StubAccepted& stub_accepted) {
            if(stub_accepted.since_last_stub > std::chrono::seconds(2)) {
                stub_accepted.since_last_stub = std::chrono::system_clock::duration::zero();

                Message message_back(MessageType::StubMessage);
                std::string s1_back("To tell you I'm sorry for everything that I've done");
                std::string s2_back("But when I call, you never seem to be home");

                message_back << s1_back << s2_back;

                write_message_queue.enqueue(std::move(message_back));
                state = WaitingForStub();
            }
        }}, state);
    }
};