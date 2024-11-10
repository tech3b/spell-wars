#pragma once

#include "../state.hpp"

class RunningGame : public GameState {
    std::chrono::system_clock::duration since_last_stub;
    bool accepted_stub;

    std::string s1;
    std::string s2;
    bool strings_written;

public:
    RunningGame() : since_last_stub(), accepted_stub(false), s1(), s2(), strings_written(true) {
    }

    virtual std::optional<std::unique_ptr<GameState>> elapsed(std::chrono::system_clock::duration& elapsed) {
        if(!strings_written) {
            std::cout << "s1: " << s1 << std::endl << "s2: " << s2 << std::endl;
            strings_written = true;
        }
        if(accepted_stub) {
            since_last_stub += elapsed;
        }
        return {};
    }

    virtual void io_updates(TFQueue<Message>& read_message_queue, TFQueue<Message>& write_message_queue) {
        for(Message& message : read_message_queue) {
            switch(message.type()) {
                case MessageType::StubMessage: {
                    std::string s1;
                    std::string s2;
                    message >> s2 >> s1;

                    this->s1 = std::move(s1);
                    this->s2 = std::move(s2);

                    strings_written = false;
                    accepted_stub = true;
                    break;
                }
            }
        }
        if(accepted_stub && since_last_stub > std::chrono::seconds(2)) {
            accepted_stub = false;
            since_last_stub -= std::chrono::seconds(2);

            Message message_back(MessageType::StubMessage);
            std::string s1_back("To tell you I'm sorry for everything that I've done");
            std::string s2_back("But when I call, you never seem to be home");

            message_back << s1_back << s2_back;

            write_message_queue.enqueue(std::move(message_back));
        }
    }
};