package org.biglittleidea.aln;

public class LocalChannel implements IChannel {
    IPacketHandler packetHandler = new IPacketHandler() {
        public void onPacket(Packet packet) { }
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
    }

    public void close() {
        
    }
}
