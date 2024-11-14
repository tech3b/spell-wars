#pragma once

#include "../state.hpp"

class RunningGame : public GameState {
    enum class State {
        WAITING_FOR_STUB,
        STUB_ACCEPTED
    };

    State state;
    bool reacted;
    std::chrono::system_clock::duration since_last_stub;
    std::string s1;
    std::string s2;

    void update_state(State updated_state) {
        state = updated_state;
        reacted = false;
    }

    void react_once(const std::function<void()>& f) {
        if(!reacted) {
            f();
            reacted = true;
        }
    }

public:
    RunningGame()
        : state(State::WAITING_FOR_STUB),
          reacted(true),
          since_last_stub(),
          s1(),
          s2() {
    }

    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed,
                                                              InputState& input_state) {
        switch(state) {
            case State::WAITING_FOR_STUB: {
                break;
            }
            case State::STUB_ACCEPTED: {
                react_once([&]() {
                    std::cout << "s1: " << s1 << std::endl << "s2: " << s2 << std::endl;
                });
                since_last_stub += elapsed;
                break;
            }
        }
        return {};
    }

    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) {
        switch(state) {
            case State::WAITING_FOR_STUB: {
                for(Message message : read_message_queue) {
                    switch(message.type()) {
                        case MessageType::StubMessage: {
                            std::string s1;
                            std::string s2;
                            message >> s2 >> s1;

                            this->s1 = std::move(s1);
                            this->s2 = std::move(s2);

                            update_state(State::STUB_ACCEPTED);
                            break;
                        }
                    }
                }
                break;
            }
            case State::STUB_ACCEPTED: {
                if(since_last_stub > std::chrono::seconds(2)) {
                    since_last_stub = std::chrono::system_clock::duration::zero();

                    Message message_back(MessageType::StubMessage);
                    std::string s1_back("To tell you I'm sorry for everything that I've done");
                    std::string s2_back("But when I call, you never seem to be home");

                    message_back << s1_back << s2_back;

                    write_message_queue.enqueue(std::move(message_back));
                    update_state(State::WAITING_FOR_STUB);
                }
                break;
            }
        }
    }
};