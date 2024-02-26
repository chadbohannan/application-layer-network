package biglittleidea.alnn.ui.wifi;

import android.content.Intent;
import android.content.res.Resources;
import android.os.Bundle;
import android.util.Log;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.widget.Toast;

import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.LifecycleOwner;

import com.google.zxing.client.android.Intents;
import com.journeyapps.barcodescanner.ScanContract;
import com.journeyapps.barcodescanner.ScanOptions;

import java.util.Set;

import biglittleidea.alnn.App;
import biglittleidea.alnn.MainActivity;
import biglittleidea.alnn.databinding.FragmentWifiBinding;

public class WifiFragment extends Fragment {

    private FragmentWifiBinding binding;

    StoredConnectionItem[] composeDirectConnectionList(Set<String> set) {
        StoredConnectionItem[] array = new StoredConnectionItem[set.size()];
        int i = 0;
        for (String value : set) {
            array[i++] = new StoredConnectionItem(value);
        }
        return array;
    }

    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        App app = App.getInstance();
        binding = FragmentWifiBinding.inflate(inflater, container, false);
        final ListView interfaceListView = binding.interfaceListView;
        final ListView discoveryListView = binding.discoveryListView;
        final ListView connectionsListView = binding.connectionsListView;
        final ListView storedConnectionsListView = binding.savedConnectionsListView;

        app.localInetInfo.observe(getViewLifecycleOwner(), localInetInfos -> {
            LifecycleOwner lco = getViewLifecycleOwner();
            interfaceListView.setAdapter(new LocalNetInfoListAdapter(getActivity(), lco, localInetInfos));
            ViewGroup.LayoutParams params = interfaceListView.getLayoutParams();

            float dip = 225f; // height of view
            Resources r = getResources();
            int px = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dip, r.getDisplayMetrics());

            params.height = px * localInetInfos.size();
            interfaceListView.setLayoutParams(params);
            interfaceListView.requestLayout();
        });

        app.beaconInfo.observe(getViewLifecycleOwner(), beaconInfos -> {
            discoveryListView.setAdapter(new BeaconInfoListAdapter(app, beaconInfos, getViewLifecycleOwner()));
            ViewGroup.LayoutParams params = discoveryListView.getLayoutParams();

            float dip = 91f;
            Resources r = getResources();
            int px = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dip, r.getDisplayMetrics());

            params.height = px * beaconInfos.size();
            discoveryListView.setLayoutParams(params);
            discoveryListView.requestLayout();
        });

        app.activeConnections.observe(getViewLifecycleOwner(), connections -> {
            connectionsListView.setAdapter(new ActiveConnectionListAdapter(app, connections, getViewLifecycleOwner()));
            ViewGroup.LayoutParams params = connectionsListView.getLayoutParams();

            float dip = 91f;
            Resources r = getResources();
            int px = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dip, r.getDisplayMetrics());

            params.height = px * connections.size();
            connectionsListView.setLayoutParams(params);
            connectionsListView.requestLayout();
        });

        app.storedConnections.observe(getViewLifecycleOwner(), connectionList -> {
            StoredConnectionItem[] array = composeDirectConnectionList(connectionList);
            StoredConnectionListAdapter adapter = new StoredConnectionListAdapter(getActivity(), array, getViewLifecycleOwner());
            storedConnectionsListView.setAdapter(adapter);

            float dip = 121f;
            Resources r = getResources();
            int px = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dip, r.getDisplayMetrics());

            ViewGroup.LayoutParams params = storedConnectionsListView.getLayoutParams();
            params.height = px * (adapter.getCount());
            storedConnectionsListView.setLayoutParams(params);
            storedConnectionsListView.requestLayout();
        });

        return binding.getRoot();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }

}