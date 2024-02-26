package biglittleidea.alnn.ui.service;

import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import java.util.ArrayList;
import java.util.List;

import biglittleidea.alnn.App;
import biglittleidea.alnn.LocalServiceHandler;
import biglittleidea.alnn.R;
import biglittleidea.alnn.databinding.FragmentServiceBinding;

public class ServiceFragment extends Fragment {
    App app = App.getInstance();

    private FragmentServiceBinding binding;


    List<LocalServiceHandler> localServices = new ArrayList<>();


    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {

        binding = FragmentServiceBinding.inflate(inflater, container, false);
        // TODO add service button
        binding.addSeviceButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Toast.makeText(getActivity(), "TODO add service", Toast.LENGTH_SHORT).show();
                makeAddServiceDialog().show();
            }
        });
        app.mldLocalServices.observe(getViewLifecycleOwner(), localServices -> {
            this.localServices = localServices;
            binding.listView.setAdapter(new LocalServiceListAdapter(getActivity(), localServices));
        });
        return binding.getRoot();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }

    private Dialog makeAddServiceDialog() {
        final Dialog dialog = new Dialog(getActivity());

        dialog.setContentView(R.layout.add_service_dialog);
        dialog.setTitle("Add New Service");
        dialog.setCancelable(true);
        dialog.findViewById(R.id.cancel_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialog.cancel();
            }
        });
        dialog.findViewById(R.id.ok_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                EditText serviceNameEdit = dialog.findViewById(R.id.service_name);
                String serviceName = serviceNameEdit.getText().toString();
                if (serviceNameEdit.length() > 0)
                    App.getInstance().addLocalService(serviceName);
                dialog.cancel();
            }
        });
        return dialog;
    }
}
