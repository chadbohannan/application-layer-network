package biglittleidea.alnn;

import android.util.Pair;

import androidx.lifecycle.MutableLiveData;

import java.util.ArrayList;
import java.util.Date;
import java.util.LinkedList;
import java.util.List;

import biglittleidea.aln.IPacketHandler;
import biglittleidea.aln.Packet;

public class LocalServiceHandler implements IPacketHandler {
    public String service;
    public LinkedList<Pair<Date,Packet>> packets = new LinkedList<>();
    public final MutableLiveData<List<Pair<Date,Packet>>> mdlPackets = new MutableLiveData<>();

    public interface OnChangedHandler {
        void changed();
    }
    OnChangedHandler onChangedHandler = null;

    public LocalServiceHandler(String service) {
        this.service = service;
    }

    public void setOnChangedHandler(OnChangedHandler och) {
        this.onChangedHandler = och;
    }

    @Override
    public void onPacket(Packet p) {
        packets.addFirst(new Pair<>(new Date(), p));
        if (packets.size() > 5)
            packets.removeLast();
        mdlPackets.postValue(packets);
        if(this.onChangedHandler != null)
            this.onChangedHandler.changed();
    }
}
