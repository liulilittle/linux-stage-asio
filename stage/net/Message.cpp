#include "Message.h"

namespace ep
{
    namespace net
    {
        std::atomic<unsigned int>                               Message::_sseqnoc(Hosting::GetTickCount());

        Message::Message()
            : CommandId(0)
            , SequenceNo(0) {
            Payload.reset();
        }

        Message::Message(unsigned short commandId, unsigned int sequenceNo, const std::shared_ptr<BufferSegment>& payload)
            : CommandId(commandId)
            , SequenceNo(sequenceNo) {
            Payload = payload;
        }

        Message::Message(const std::shared_ptr<BufferSegment>& payload)
            : CommandId(0)
            , SequenceNo(0) {
            Reset(payload);
        }

        Message::~Message() {
            CommandId = 0;
            SequenceNo = 0;
            Payload.reset();
        }

        unsigned int Message::NewId() {
            unsigned int sequence = 0;
            do {
                sequence = ++_sseqnoc;
            } while (0 == sequence);
            return sequence;
        }

        std::shared_ptr<Message> Message::Create(unsigned short commandId, unsigned int sequenceNo, const std::shared_ptr<BufferSegment>& payload) {
            std::shared_ptr<Message> message = make_shared_object<Message>(commandId, sequenceNo, payload);
            return message;
        }

        void Message::Reset(const std::shared_ptr<BufferSegment>& payload) {
            Payload = (payload);
        }
    }
}