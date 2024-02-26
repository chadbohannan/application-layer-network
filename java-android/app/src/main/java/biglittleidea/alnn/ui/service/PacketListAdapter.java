package biglittleidea.alnn.ui.service;

import android.app.Activity;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import biglittleidea.aln.Packet;
import biglittleidea.alnn.R;

public class PacketListAdapter extends BaseAdapter {
    LayoutInflater inflter;
    List<Pair<Date,Packet>> list = new ArrayList<>();
    Activity activity;

    public static final String TextType = "text";
    public static final String BinaryType = "binary";
    String type;
    public PacketListAdapter(Activity activity, String type, List<Pair<Date,Packet>> packets) {
        this.activity = activity;
        this.type = type;
        for (Pair<Date,Packet> item : packets)
            this.list.add(item);
        inflter = (LayoutInflater.from(activity));
    }

    @Override
    public int getCount() {
        return list.size();
    }

    @Override
    public Pair<Date,Packet> getItem(int position) {
        return list.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View view, ViewGroup parent) {
        Pair<Date,Packet> info = list.get(position);

        view = inflter.inflate(R.layout.local_service_log_item, null);

        TextView dateText = view.findViewById(R.id.dateText);
        String metaData = String.format("  (%d bytes)", info.second.Data.length);
        dateText.setText(info.first.toString() + metaData);

        TextView srcText = view.findViewById(R.id.srcText);
        srcText.setText(info.second.SourceAddress);

        String content;
        if (type.equals(TextType)) {
            content = new String(info.second.Data, StandardCharsets.UTF_8);
        } else {
            StringBuilder builder = new StringBuilder();
            for (byte b : info.second.Data) {
                builder.append(String.format("%x ", b).toUpperCase());
            }
            content = builder.toString();
        }

        TextView contentText = view.findViewById(R.id.contentText);
        contentText.setText(content);
        return view;
    }
}
