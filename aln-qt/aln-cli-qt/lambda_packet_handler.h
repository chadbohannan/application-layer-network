#ifndef LAMBDA_PACKET_HANDLER_H
#define LAMBDA_PACKET_HANDLER_H

#include <functional>
#include <aln/router.h>

// Helper class to wrap lambda functions as PacketHandlers for testing
class LambdaPacketHandler : public PacketHandler
{
public:
    using HandlerFunc = std::function<void(Packet*)>;

    explicit LambdaPacketHandler(HandlerFunc func) : handler(func) {}

    void onPacket(Packet* p) override {
        if (handler) {
            handler(p);
        }
    }

private:
    HandlerFunc handler;
};

#endif // LAMBDA_PACKET_HANDLER_H
