package biglittleidea.alnn.ui.mesh;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;

import biglittleidea.aln.Router;
import biglittleidea.alnn.App;
import biglittleidea.alnn.R;

/**
 NodeListAdapter is for the vertical list of nodes on the network
**/

public class NodeListAdapter extends BaseAdapter {
    LayoutInflater inflter;
    List<Router.NodeInfoItem> list = new ArrayList<>();
    Activity activity;
    public NodeListAdapter(Activity activity, Map<String, Router.NodeInfoItem> nodes) {
        this.activity = activity;
        for (Router.NodeInfoItem item : nodes.values())
            this.list.add(item);
        inflter = (LayoutInflater.from(activity));
    }

    @Override
    public int getCount() {
        return list.size();
    }

    @Override
    public Router.NodeInfoItem getItem(int position) {
        return list.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View view, ViewGroup parent) {
        Router.NodeInfoItem info = list.get(position);
        view = inflter.inflate(R.layout.mesh_node_item, null);
        TextView text = view.findViewById(R.id.titleText);
        text.setText(info.address);
        ListView listView = view.findViewById(R.id.serviceListView);
        listView.setAdapter(new NodeServiceListAdapter(activity, info.address, info.services));
        return view;
    }
}
