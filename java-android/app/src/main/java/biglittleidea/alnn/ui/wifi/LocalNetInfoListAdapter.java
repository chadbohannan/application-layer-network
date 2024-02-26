package biglittleidea.alnn.ui.wifi;

import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.BaseAdapter;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.Switch;
import android.widget.TextView;

import androidx.lifecycle.LifecycleOwner;

import java.util.List;

import biglittleidea.alnn.App;
import biglittleidea.alnn.LocalInetInfo;
import biglittleidea.alnn.R;

public class LocalNetInfoListAdapter extends BaseAdapter {
    LayoutInflater inflter;
    List<LocalInetInfo> list;
    Activity activity;
    LifecycleOwner lifecycleOwner;
    public LocalNetInfoListAdapter(Activity activity, LifecycleOwner lifecycleOwner, List<LocalInetInfo> list) {
        this.list = list;
        this.activity = activity;
        this.lifecycleOwner = lifecycleOwner;
        inflter = (LayoutInflater.from(activity));
    }

    @Override
    public int getCount() {
        return list.size();
    }

    @Override
    public LocalInetInfo getItem(int position) {
        return list.get(position);
    }

    @Override
    public long getItemId(int position) {
        return list.get(position).name.hashCode();
    }

    @Override
    public View getView(int position, View _view, ViewGroup parent) {
        LocalInetInfo info = list.get(position);
        String iface = info.inetAddress.toString();

        final View view = inflter.inflate(R.layout.wifi_iface_item, null);
        ((TextView)view.findViewById(R.id.iface_name_view)).setText(info.name);
        ((TextView)view.findViewById(R.id.iface_address_view)).setText(iface);
        ((TextView)view.findViewById(R.id.bcast_listen_address_view)).setText(info.bcastAddress.toString());

        App app = App.getInstance();

        boolean actAsHost = app.getActAsHostForInterface(iface);
        view.findViewById(R.id.udp_bcast_listen_layout).setVisibility(actAsHost ? View.GONE : View.VISIBLE);
        view.findViewById(R.id.tcp_host_listen_layout).setVisibility(actAsHost ? View.VISIBLE : View.GONE);
        view.findViewById(R.id.udp_advertise_layout).setVisibility(actAsHost ? View.VISIBLE : View.GONE);

        Switch actAsHostSwitch = view.findViewById(R.id.act_as_host_switch);
        actAsHostSwitch.setChecked(actAsHost);
        actAsHostSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                app.setActAsHostForInterface(iface, isChecked);
                view.findViewById(R.id.udp_bcast_listen_layout).setVisibility(isChecked ? View.GONE : View.VISIBLE);
                view.findViewById(R.id.tcp_host_listen_layout).setVisibility(isChecked ? View.VISIBLE : View.GONE);
                view.findViewById(R.id.udp_advertise_layout).setVisibility(isChecked ? View.VISIBLE : View.GONE);
            }
        });

        // TCP Listen
        short tcpHostPort = app.getTcpHostPortForInterface(iface);
        Switch tcpListenSwitch = view.findViewById(R.id.tcp_listen_switch);
        tcpListenSwitch.setChecked(app.isListeningToTCP(info.inetAddress, tcpHostPort));
        tcpListenSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                app.listenToTCP(info.inetAddress, tcpHostPort, isChecked);
            }
        });
        TextView tcpPortText = view.findViewById(R.id.tcp_port_view);
        tcpPortText.setText(String.format("port %d", tcpHostPort));
        tcpPortText.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                makeSetPortDialog(iface, "tcp_host").show();
            }
        });
        view.findViewById(R.id.help_tcp_listen).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String msgHtml = // TODO load from html file in assets folder
                        "Activate Listen/Serve to start a TCP server socket that will accept " +
                        "connections from other ALN components.";
                makeShowHelpDialog("Listen/Serve", msgHtml).show();
            }
        });

        // UDP advertisement
        short udpBcastPort = app.getUdpAdvertPortForInterface(iface);
        Switch udpBroadcastSwitch = view.findViewById(R.id.udp_advertise_switch);
        udpBroadcastSwitch.setChecked(app.isBroadcastingUDP(info.bcastAddress, udpBcastPort));
        udpBroadcastSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                String message = String.format("tcp+aln:/%s:%d", info.inetAddress, tcpHostPort);
                app.broadcastUDP(info.bcastAddress, udpBcastPort, message, isChecked);
            }
        });
        short udpAdvertPort = app.getUdpAdvertPortForInterface(iface);
        TextView udpAdvertPortText = view.findViewById(R.id.udp_advertise_port_view);
        udpAdvertPortText.setText(String.format("port %d", udpAdvertPort));
        udpAdvertPortText.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                makeSetPortDialog(iface, "udp_advert").show();
            }
        });
        view.findViewById(R.id.help_udp_advertise).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String msgHtml = // TODO load from html file in assets folder
                    "Activate Broadcast Advertise to broadcast a UDP datagram containing the " +
                    "connection string needed for other ALN components on the local network to " +
                    "discover this component and (optionally) make a connection." +
                    "This application can only host <b>tcp-aln</b> connections.<br/>";
                makeShowHelpDialog("Broadcast Advertise", msgHtml).show();
            }
        });

        // act as client
        short udpListenPort = app.getUdpAdvertPortForInterface(iface);
        Switch udpToggle = view.findViewById(R.id.udp_listen_switch);
        udpToggle.setChecked(app.isListeningToUDP(info.bcastAddress, udpListenPort));
        udpToggle.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                short port = app.getUdpAdvertPortForInterface(iface);
                app.listenToUDP(info.bcastAddress, port, isChecked);
            }
        });
        TextView udpListenPortText = view.findViewById(R.id.udp_listen_port_view);
        udpListenPortText.setText(String.format("port %d", udpAdvertPort));
        udpListenPortText.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                makeSetPortDialog("iface", "udp_listen").show();
            }
        });
        view.findViewById(R.id.help_udp_listen).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String broadcastListenHtml = // TODO load from html file in assets folder
                    "Activate Broadcast Listen to receive UDP datagram advertisements shared by other ALN components on your local network.<br/>" +
                    "Supported protocols are:<br/><b>tcp-aln, tls-aln, tcp-maln, tls-maln</b><br/>" +
                    "Hosts advertising supported protocols will be listed in Discovered Hosts.<br/>" +
                    "Use the Discovered Hosts interface to create a connection.<br/>" +
                    "Visit <a href=\"http://https://biglittleidea.org/article/5179846254657536\">ALN channels</a> for more information.";

                makeShowHelpDialog("Broadcast Listen", broadcastListenHtml).show();
            }
        });
        return view;
    }


    Dialog makeSetPortDialog(String iface, String portType) {
        final Dialog dialog = new Dialog(activity);
        LayoutInflater inflater = activity.getLayoutInflater();
        View view = inflater.inflate(R.layout.set_listen_port_dialog, null);
        dialog.setContentView(view);
        dialog.setTitle("Configuration");

        TextView titleView = view.findViewById(R.id.titleLabel);
        if (portType == "udp_advert") {
            titleView.setText("Broadcast on port");
        } else {
            titleView.setText("Listen on port");
        }

        App app = App.getInstance();
        short port;
        switch (portType) {
            case "udp_advert":
            case "udp_listen": // TODO save separately
                port = app.getUdpAdvertPortForInterface(iface);
                break;
            default:
                port = app.getTcpHostPortForInterface(iface);
        }

        EditText portNumEdit =  view.findViewById(R.id.portNumEdit);
        portNumEdit.setText(String.format("%d", port));

        view.findViewById(R.id.cancel_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialog.cancel();
            }
        });

        view.findViewById(R.id.ok_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String portText = portNumEdit.getText().toString();
                short port = Short.parseShort(portText);
                if (portType == "udp_advert") {
                    App.getInstance().setUdpAdvertPortForInterface(iface, port);
                } else {
                    App.getInstance().setTcpHostPortForInterface(iface, port);
                }
                dialog.dismiss();
            }
        });
        return dialog;
    }

    Dialog makeShowHelpDialog(String title, String msg) {

        final Dialog dialog = new Dialog(activity);
        LayoutInflater inflater = activity.getLayoutInflater();
        View view = inflater.inflate(R.layout.show_topic_help_dialog, null);
        dialog.setContentView(view);
        dialog.setTitle("Help");

        TextView titleView = view.findViewById(R.id.titleLabel);
        titleView.setText(title);

        WebView contentView = view.findViewById(R.id.contentLabel);
        contentView.loadData(msg, "text/html", "utf-8");

        view.findViewById(R.id.ok_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialog.dismiss();
            }
        });
        return dialog;
    }
};