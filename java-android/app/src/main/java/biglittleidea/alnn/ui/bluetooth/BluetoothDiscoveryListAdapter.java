package biglittleidea.alnn.ui.bluetooth;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.os.ParcelUuid;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

import biglittleidea.alnn.App;
import biglittleidea.alnn.R;

public class BluetoothDiscoveryListAdapter extends BaseAdapter {

    private LayoutInflater inflter;
    private List<BluetoothService> deviceList = new ArrayList<>();

    protected class BluetoothService {
        protected BluetoothDevice device;
        protected String uuid;
        protected BluetoothService(BluetoothDevice device, String uuid) {
            this.device = device;
            this.uuid = uuid;
        }
        public String toString() {
            return String.format("%s-%s", device.getAddress(), uuid);
        }
    }

    @SuppressLint("MissingPermission")
    public BluetoothDiscoveryListAdapter(List<BluetoothDevice> deviceList) {
        inflter = LayoutInflater.from(App.getInstance());
        for (BluetoothDevice device : deviceList) {
            this.deviceList.add(new BluetoothService(device, device.getAddress()));
        }
    }

    @Override
    public int getCount() {
        return deviceList.size();
    }

    @Override
    public BluetoothService getItem(int position) {
        return deviceList.get(position);
    }

    @Override
    public long getItemId(int position) {
        return deviceList.get(position).toString().hashCode();
    }

    @SuppressLint("MissingPermission")
    @Override
    public View getView(int position, View view, ViewGroup parent) {
        view = inflter.inflate(R.layout.bluetooth_discovery_item, null);

        final BluetoothService service = deviceList.get(position);
        final String addr = service.device.getAddress();
        final String name = service.device.getName();

        TextView nameText = view.findViewById(R.id.nameText);
        nameText.setText(String.format("%s %s", addr, name));

        final boolean isConnected = App.getInstance().isConnected(service.device);
        if (isConnected) {
            view.findViewById(R.id.icon_black).setVisibility(View.VISIBLE);
            view.findViewById(R.id.icon_grey).setVisibility(View.INVISIBLE);
        } else {
            view.findViewById(R.id.icon_black).setVisibility(View.INVISIBLE);
            view.findViewById(R.id.icon_grey).setVisibility(View.VISIBLE);
        }
        view.setOnClickListener(v -> {
            Toast.makeText(App.getInstance(), "on click:"+ name, Toast.LENGTH_SHORT).show();
            App.getInstance().connectTo(service.device, !isConnected);
        });

        return view;
    }
}
