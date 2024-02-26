package biglittleidea.alnn.ui.service;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.util.List;

import biglittleidea.alnn.App;
import biglittleidea.alnn.LocalServiceHandler;
import biglittleidea.alnn.R;

/**
 NodeListAdapter is for the vertical list of nodes on the network
**/

public class LocalServiceListAdapter extends BaseAdapter {
    LayoutInflater inflter;
    List<LocalServiceHandler> list;
    Activity activity;
    public LocalServiceListAdapter(Activity activity, List<LocalServiceHandler> localServices) {
        this.activity = activity;
        this.list = localServices;
        inflter = (LayoutInflater.from(activity));
    }

    @Override
    public int getCount() {
        return list.size();
    }

    @Override
    public LocalServiceHandler getItem(int position) {
        return list.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View view, ViewGroup parent) {
        LocalServiceHandler info = list.get(position);
        view = inflter.inflate(R.layout.local_service_item, null);
        TextView text = view.findViewById(R.id.titleText);
        String name = String.format("%s, %d msgs", info.service, info.packets.size());
        text.setText(name);

        ListView listView = view.findViewById(R.id.serviceListView);
        listView.setAdapter(new PacketListAdapter(activity, "text", info.packets));

        final AlertDialog.Builder builder;
        builder = new AlertDialog.Builder(activity);
        ImageView trashButton = view.findViewById(R.id.trash_icon);
        trashButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                builder.setMessage("Discontinue service?")
                    .setCancelable(true)
                    .setPositiveButton("Confirm", new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            App.getInstance().removeLocalService(info.service);
                            Toast.makeText(activity.getApplicationContext(), "stopping service", Toast.LENGTH_SHORT).show();
                        }
                    })
                    .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            dialog.cancel();
                        }
                    }).show();
            }
        });

        return view;
    }
}
