package biglittleidea.alnn.ui.mesh;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import biglittleidea.aln.Router;
import biglittleidea.alnn.App;
import biglittleidea.alnn.R;

public class NodeServiceListAdapter extends BaseAdapter {
    LayoutInflater inflter;
    List<Router.ServiceListItem> list = new ArrayList<>();
    Activity activity;
    String address;
    public NodeServiceListAdapter(Activity activity, String address, Set<Router.ServiceListItem> services) {
        this.activity = activity;
        this.address = address;
        for (Router.ServiceListItem item : services)
            this.list.add(item);
        inflter = (LayoutInflater.from(activity));
    }

    @Override
    public int getCount() {
        return list.size();
    }

    @Override
    public Router.ServiceListItem getItem(int position) {
        return list.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View view, ViewGroup parent) {
        Router.ServiceListItem info = list.get(position);
        view = inflter.inflate(R.layout.service_node_item, null);
        TextView text = view.findViewById(R.id.service_name_view);
        text.setText(info.service);

        SendPacketActionItem[] nodeActions;
        Set<String> actions = App.getInstance().getActionsForService(info.service);
        if (actions != null) {
            nodeActions = new SendPacketActionItem[actions.size()];
            Object[] strings = actions.toArray();
            for (int i = 0; i < nodeActions.length; i++) {
                String[] parts = ((String) strings[i]).split(("\t"));
                if (parts.length == 0) {
                    nodeActions[i] = new SendPacketActionItem(address, info.service, "", "");
                } else  if (parts.length == 2) {
                    nodeActions[i] = new SendPacketActionItem(address, info.service, parts[1], parts[0]);
                } else {
                    nodeActions[i] = new SendPacketActionItem(address, info.service, parts[0], parts[0]);
                }
            }
        } else {
            nodeActions = new SendPacketActionItem[0];
        }

        RecyclerView rv = view.findViewById(R.id.packet_action_list);
        rv.setAdapter(new PacketButtonListAdapter(activity, info.service, nodeActions));
        rv.setLayoutManager(new LinearLayoutManager(activity, LinearLayoutManager.HORIZONTAL, false));
        return view;
    }
}
