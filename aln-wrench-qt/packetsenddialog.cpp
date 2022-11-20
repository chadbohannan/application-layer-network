#include "packetsenddialog.h"

#include <QHBoxLayout>
#include <QLabel>

#include <QPushButton>

class ResponseHandler : public PacketHandler {
    PacketSendDialog* sendDialog;
public:
    ResponseHandler(PacketSendDialog* dlg) : sendDialog(dlg) {}
    void onPacket(Packet* p) {
        sendDialog->setResponse(p->data);
    }
};

PacketSendDialog::PacketSendDialog(Router* alnRouter, QWidget *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags), router(alnRouter) {
    setWindowTitle("Send Packet");
    contextID = alnRouter->registerContextHandler(new ResponseHandler(this));
    setLayout(createLayout());
    connect(this, SIGNAL(finished(int)), SLOT(onClose()));
}

void PacketSendDialog::setDest(QString destAddress) {
    destLineEdit->setText(destAddress);
}

void PacketSendDialog::setService(QString serviceName) {
    serviceLineEdit->setText(serviceName);
}

void PacketSendDialog::setData(QString data) {
    dataLineEdit->setText(data);
}

void PacketSendDialog::setResponse(QString resp) {
    responseLineEdit->setText(resp);
}

void PacketSendDialog::onClose() {
    router->releaseContext(contextID);
}

void PacketSendDialog::onSendClicked() {
    QString dst = destLineEdit->text();
    QString srv = serviceLineEdit->text();
    QString data = dataLineEdit->text();
    Packet* packet = new Packet(dst, srv, contextID, data.toUtf8());
    qInfo() << "Sending " << packet->toString();
    router->send(packet);
}

void PacketSendDialog::onCloseClicked() {
    close();
}

QLayout* PacketSendDialog::createLayout() {
    QHBoxLayout* destLayout = new QHBoxLayout;
    destLineEdit = new QLineEdit;
    destLayout->addWidget(new QLabel("dst"));
    destLayout->addWidget(destLineEdit);

    QHBoxLayout* serviceLayout = new QHBoxLayout;
    serviceLineEdit = new QLineEdit;
    serviceLayout->addWidget(new QLabel("srv"));
    serviceLayout->addWidget(serviceLineEdit);

    QHBoxLayout* contextLayout = new QHBoxLayout;
    contextLayout->addWidget(new QLabel("ctx"));
    contextLayout->addWidget(new QLabel(QString("%1").arg(contextID)));
    contextLayout->addStretch();

    QHBoxLayout* dataLayout = new QHBoxLayout;
    dataLineEdit = new QLineEdit;
    dataLayout->addWidget(new QLabel("data"));
    dataLayout->addWidget(dataLineEdit);

    QHBoxLayout* responseLayout = new QHBoxLayout;
    responseLineEdit = new QLineEdit;
    responseLineEdit ->setReadOnly(true);
    responseLayout->addWidget(new QLabel("resp"));
    responseLayout->addWidget(responseLineEdit );

    QPushButton* sendButton = new QPushButton("Send");
    connect(sendButton, SIGNAL(clicked()), this, SLOT(onSendClicked()));

    QPushButton* closeButton = new QPushButton("Close");
    connect(closeButton, SIGNAL(clicked()), this, SLOT(onCloseClicked()));

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    buttonLayout->addWidget(sendButton);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addLayout(destLayout);
    layout->addLayout(serviceLayout);
    layout->addLayout(contextLayout);
    layout->addLayout(dataLayout);
    layout->addLayout(responseLayout);
    layout->addLayout(buttonLayout);
    return layout;
}
