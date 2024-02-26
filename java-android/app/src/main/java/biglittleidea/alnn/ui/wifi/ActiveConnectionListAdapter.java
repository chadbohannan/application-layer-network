package biglittleidea.alnn.ui.wifi;

import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.Switch;
import android.widget.TextView;

import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.Observer;

import java.util.List;

import biglittleidea.alnn.App;
import biglittleidea.alnn.BeaconInfo;
import biglittleidea.alnn.R;

public class ActiveConnectionListAdapter extends BaseAdapter {
    LayoutInflater inflter;
    List<String> list;
    App app;
    LifecycleOwner lifecycleOwner;
    public ActiveConnectionListAdapter(App app, List<String> list, LifecycleOwner lifecycleOwner) {
        this.app = app;
        this.list = list;
        inflter = (LayoutInflater.from(app));
        this.lifecycleOwner = lifecycleOwner;
    }

    @Override
    public int getCount() {
        return list.size();
    }

    @Override
    public String getItem(int position) {
        return list.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View view, ViewGroup parent) {
        String info = list.get(position);
        view = inflter.inflate(R.layout.wifi_connection_item, null);
        ImageView icon = view.findViewById(R.id.icon);
        icon.setImageResource(R.drawable.connected);

        TextView addressText = view.findViewById(R.id.addressTextView);
        addressText.setText(info);

        Button disconnectButton = view.findViewById(R.id.disconnect_button);
        disconnectButton.setOnClickListener(v -> {
            app.disconnectActiveConnection(info);
        });
        return view;
    }
}
