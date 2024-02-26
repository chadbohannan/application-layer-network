package biglittleidea.alnn.ui.mesh;
import biglittleidea.aln.Packet;
import biglittleidea.alnn.App;
import biglittleidea.alnn.R;

import android.app.Activity;
import android.app.Dialog;
import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.recyclerview.widget.RecyclerView;

import java.nio.charset.StandardCharsets;

public class PacketButtonListAdapter extends RecyclerView.Adapter<PacketButtonListAdapter.ViewHolder> {

    private Activity activity;
    private String service;
    private SendPacketActionItem[] data;

    public static class ViewHolder extends RecyclerView.ViewHolder {
        private final View view;
        public ViewHolder(View view) {
            super(view);
            this.view = view;
        }

        public TextView getTextView() {
            return view.findViewById(R.id.text_view);
        }

        public ImageView getImageView() {
            return view.findViewById(R.id.image_view);
        }
    }

    public PacketButtonListAdapter(Activity activity, String service, SendPacketActionItem[] data) {
        this.activity = activity;
        this.service = service;
        this.data = data;
    }

    @Override // Create new views (invoked by the layout manager)
    public ViewHolder onCreateViewHolder(ViewGroup viewGroup, int viewType) {
        View view = LayoutInflater.from(viewGroup.getContext())
                .inflate(R.layout.service_action_item, viewGroup, false);

        return new ViewHolder(view);
    }

    @Override // Replace the contents of a view (invoked by the layout manager)
    public void onBindViewHolder(ViewHolder viewHolder, final int position) {
        if (position >= data.length) {
            viewHolder.getImageView().setImageResource(R.drawable.icons8_plus);
            viewHolder.getTextView().setText("New Packet Button");
        } else {
            SendPacketActionItem item = data[position];
            String text = item.title;
            int imgResource = R.drawable.packet;
            int bgColor = 0;
            if (item.action.contains("#")) {
                String[] parts = item.action.split(" ");
                if (parts.length == 2 && parts[1].startsWith("#")) {
                    imgResource = 0;
                    bgColor = Color.parseColor(parts[1]);
                }
            }
            viewHolder.getTextView().setText(text);
            viewHolder.getImageView().setImageResource(imgResource);
            viewHolder.getImageView().setBackgroundColor(bgColor);
        }
        viewHolder.view.setOnClickListener(onClickListener(position));
        viewHolder.view.setOnLongClickListener(onLongClickListener(position));
    }

    @Override // Return the size of your dataset (invoked by the layout manager)
    public int getItemCount() {
        return data.length + 1;
    }

    private Dialog makeButtonEditDialog(int position) {
        final Dialog dialog = new Dialog(activity);

        dialog.setContentView(R.layout.add_packet_button_dialog);
        dialog.setTitle("Position" + position);
        dialog.setCancelable(true);
        dialog.findViewById(R.id.cancel_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialog.cancel();
            }
        });
        return dialog;
    }

    private View.OnClickListener onClickListener(final int position){
        return new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (position >= PacketButtonListAdapter.this.data.length) {
                    Dialog dialog  = makeButtonEditDialog(position);
                    dialog.findViewById(R.id.ok_button).setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            EditText titleEdit = dialog.findViewById(R.id.titleEdit);
                            EditText contentEdit = dialog.findViewById(R.id.contentEdit);

                            String title = titleEdit.getText().toString();
                            String content = contentEdit.getText().toString();

                            App.getInstance().saveActionItem(service, title, content);
                            titleEdit.setText("");
                            contentEdit.setText("");
                            dialog.dismiss();
                        }
                    });
                    dialog.show();
                } else { // send packet as action
                    SendPacketActionItem item = PacketButtonListAdapter.this.data[position];
                    Packet p = new Packet();
                    p.DestAddress = item.address;
                    p.Service = item.service;
                    p.Data = item.action.getBytes(StandardCharsets.UTF_8);
                    App.getInstance().send(p);
                }
            }
        };
    }


    private View.OnLongClickListener onLongClickListener(final int position){
        return new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                if (position < PacketButtonListAdapter.this.data.length) {
                    SendPacketActionItem item = PacketButtonListAdapter.this.data[position];
                    Dialog dialog  = makeButtonEditDialog(position);
                    ((TextView)dialog.findViewById(R.id.titleEdit)).setText(item.title);
                    ((TextView)dialog.findViewById(R.id.contentEdit)).setText(item.action);
                    dialog.findViewById(R.id.ok_button).setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            EditText titleEdit = dialog.findViewById(R.id.titleEdit);
                            EditText contentEdit = dialog.findViewById(R.id.contentEdit);

                            String title = titleEdit.getText().toString();
                            String content = contentEdit.getText().toString();

                            App.getInstance().replaceActionItem(service, item.title, item.action, title, content);
                            titleEdit.setText("");
                            contentEdit.setText("");
                            dialog.dismiss();
                        }
                    });
                    dialog.show();
                }
                return true;
            }
        };
    }
}
