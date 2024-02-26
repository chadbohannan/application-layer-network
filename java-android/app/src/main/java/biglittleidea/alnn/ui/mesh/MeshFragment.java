package biglittleidea.alnn.ui.mesh;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import java.util.Map;

import biglittleidea.aln.Router;
import biglittleidea.alnn.App;
import biglittleidea.alnn.databinding.FragmentMeshBinding;

public class MeshFragment extends Fragment {
    private FragmentMeshBinding binding;
    Map<String, Router.NodeInfoItem> nodeInfo;


    public View onCreateView(@NonNull LayoutInflater inflater,
                             ViewGroup container, Bundle savedInstanceState) {
        App app = App.getInstance();
        binding = FragmentMeshBinding.inflate(inflater, container, false);
        binding.localAddress.setText(app.alnRouter.getAddress());
        app.mldNodeInfo.observe(getViewLifecycleOwner(), nodeInfos -> {
            nodeInfo = nodeInfos;
            binding.listView.setAdapter(new NodeListAdapter(getActivity(), nodeInfo));
        });
        return binding.getRoot();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }
}