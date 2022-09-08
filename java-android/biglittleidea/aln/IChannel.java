package biglittleidea.aln;

public interface IChannel {
    void send(Packet p);
    void receive(IPacketHandler ph, IChannelCloseHandler ch);
    void close();
}
