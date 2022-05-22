package org.biglittleidea.aln;

import java.util.ArrayList;

public class LocalChannel implements IChannel {
    ArrayList<Packet> queue = new ArrayList<Packet>();
    IPacketHandler packetHandler = new IPacketHandler() {
        public void onPacket(Packet packet) {
            queue.add(packet);
        }
    };

    LocalChannel loopBack;
    public LocalChannel() {
        loopBack = new LocalChannel(this);
    }

    public LocalChannel LoopBack() {
        return loopBack;
    }

    public LocalChannel(LocalChannel lb) {
        loopBack = lb;
    }

    public void send(Packet p) {
        loopBack.packetHandler.onPacket(p);
    }

    public void receive(IPacketHandler packetHandler, IChannelCloseHandler closeHandler) {
        this.packetHandler = packetHandler;
        if (queue.size() > 0) {
            for (Packet p : queue)
                packetHandler.onPacket(p);
            queue.clear();
        }
    }

    public void close() {
        
    }
}
