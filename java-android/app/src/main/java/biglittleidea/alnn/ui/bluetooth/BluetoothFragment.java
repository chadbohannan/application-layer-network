package biglittleidea.alnn.ui.bluetooth;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.ParcelUuid;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.fragment.app.Fragment;

import java.util.ArrayList;
import java.util.List;

import biglittleidea.aln.BleUartSerial;
import biglittleidea.alnn.App;
import biglittleidea.alnn.databinding.FragmentBluetoothBinding;
@SuppressLint("MissingPermission")
public class BluetoothFragment extends Fragment {

    private BluetoothAdapter mBluetoothAdapter;
    private int REQUEST_ENABLE_BT = 1;
    private Handler mHandler;
    private static final long SCAN_PERIOD = 10000;
    private BluetoothLeScanner mLEScanner = null;
    private ScanSettings settings;
    private List<ScanFilter> filters;


    private static String[] PERMISSIONS_BLUETOOTH = {
            Manifest.permission.BLUETOOTH_SCAN,
            Manifest.permission.BLUETOOTH_CONNECT,
    };
    private static final int REQUEST_BLUETOOTH = 1;

    private FragmentBluetoothBinding binding;
    App app = App.getInstance();

    void initBluetooth() {
        mHandler = new Handler();
        Activity activity = getActivity();
        if (!activity.getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(activity, "BLE Not Supported", Toast.LENGTH_SHORT).show();
            return;
        } else if (ActivityCompat.checkSelfPermission(getContext(), Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(getActivity(), PERMISSIONS_BLUETOOTH, REQUEST_BLUETOOTH);
        }
        final BluetoothManager bluetoothManager =
                (BluetoothManager) activity.getSystemService(Context.BLUETOOTH_SERVICE);
        mBluetoothAdapter = bluetoothManager.getAdapter();
    }

    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        initBluetooth();
        binding = FragmentBluetoothBinding.inflate(inflater, container, false);
        View root = binding.getRoot();

        final TextView textView = binding.textBluetooth;
        final Button startButton = binding.scanButton;
        final ListView listView = binding.discoveredDevicesList;
        app.bluetoothDevices.observe(getViewLifecycleOwner(), devices -> {
            listView.setAdapter(new BluetoothDiscoveryListAdapter(devices));
            String msg = String.format("%d bluetooth devices found", devices.size());
            textView.setText(msg);
        });

        app.bluetoothDiscoveryStatus.observe(getViewLifecycleOwner(), status -> {
            textView.setText(status);
        });

        app.bluetoothDiscoveryIsActive.observe(getViewLifecycleOwner(), isActive -> {
            if (isActive) {
                startButton.setText("Stop Scan");
            } else {
                startButton.setText("Restart Scan");
            }
        });

        startButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (ActivityCompat.checkSelfPermission(getContext(), Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
                    ActivityCompat.requestPermissions(getActivity(), PERMISSIONS_BLUETOOTH, REQUEST_BLUETOOTH);
                } else {
//                    app.toggleBluetoothDiscovery();
                }
            }
        });
        return root;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mBluetoothAdapter == null || !mBluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        } else {
            mLEScanner = mBluetoothAdapter.getBluetoothLeScanner();
            settings = new ScanSettings.Builder()
                    .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                    .build();
            filters = new ArrayList<>();
            ParcelUuid uuid = ParcelUuid.fromString(BleUartSerial.SERVICE_UUID.toString());
            filters.add(new ScanFilter.Builder().setServiceUuid(uuid).build());
            scanLeDevice(true);
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mBluetoothAdapter != null && mBluetoothAdapter.isEnabled()) {
            scanLeDevice(false);
        }
    }

    private void scanLeDevice(final boolean enable) {
        if (enable) {
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    mLEScanner.stopScan(mScanCallback);
                }
            }, SCAN_PERIOD);
            mLEScanner.startScan(filters, settings, mScanCallback);
            Log.d("ALNN", "starting scan");
        } else {
            mLEScanner.stopScan(mScanCallback);
            Log.d("ALNN", "stopping scan");
        }
    }

    private ScanCallback mScanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            List<ParcelUuid> sids = result.getScanRecord().getServiceUuids();
            if (sids != null) {
                BluetoothDevice btDevice = result.getDevice();
                if (app.addBLEDevice(btDevice)) {
                    for (ParcelUuid sid : sids) {
                        Log.i("ALNN", String.format("onScanResult: address: %s, name:%s, service: %s", btDevice.getAddress(), btDevice.getName(), sid));
                    }
                }
            }
        }

        @Override
        public void onBatchScanResults(List<ScanResult> results) {
            for (ScanResult sr : results) {
                Log.i("ALNN", "ScanResult - Results:"+ sr.toString());
            }
        }

        @Override
        public void onScanFailed(int errorCode) {
            Log.e("ALNN", "Scan Failed, "+ "Error Code: " + errorCode);
        }
    };

}
