#include "startadvertisementdialog.h"

StartAdvertisementDialog::StartAdvertisementDialog(QStringList connectionStrings, QString defaultBroadcastAddr, int defaultBroadcastPort, QWidget *parent)
    : QDialog(parent), availableConnectionStrings(connectionStrings)
{
    setWindowTitle("Start Advertisement");
    setModal(true);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Horizontal layout for Host Interface, Port, and Period controls
    QHBoxLayout* topControlsLayout = new QHBoxLayout();

    // Host Interface
    QVBoxLayout* hostInterfaceLayout = new QVBoxLayout();
    QLabel* broadcastAddrLabel = new QLabel("Host Interface:", this);
    broadcastAddressLineEdit = new QLineEdit(this);
    broadcastAddressLineEdit->setText(defaultBroadcastAddr);
    broadcastAddressLineEdit->setPlaceholderText("e.g., 192.168.1.255");
    hostInterfaceLayout->addWidget(broadcastAddrLabel);
    hostInterfaceLayout->addWidget(broadcastAddressLineEdit);
    topControlsLayout->addLayout(hostInterfaceLayout);

    // Port
    QVBoxLayout* portLayout = new QVBoxLayout();
    QLabel* broadcastPortLabel = new QLabel("Port:", this);
    broadcastPortSpinBox = new QSpinBox(this);
    broadcastPortSpinBox->setMinimum(1024);
    broadcastPortSpinBox->setMaximum(65535);
    broadcastPortSpinBox->setValue(defaultBroadcastPort);
    portLayout->addWidget(broadcastPortLabel);
    portLayout->addWidget(broadcastPortSpinBox);
    topControlsLayout->addLayout(portLayout);

    // Period
    QVBoxLayout* periodLayout = new QVBoxLayout();
    QLabel* periodLabel = new QLabel("Period:", this);
    periodSpinBox = new QSpinBox(this);
    periodSpinBox->setMinimum(1);
    periodSpinBox->setMaximum(300);
    periodSpinBox->setValue(5);
    periodSpinBox->setSuffix(" sec");
    periodLayout->addWidget(periodLabel);
    periodLayout->addWidget(periodSpinBox);
    topControlsLayout->addLayout(periodLayout);

    mainLayout->addLayout(topControlsLayout);

    // Connection string helper
    QLabel* helperLabel = new QLabel("Quick Fill from Active Connections:", this);
    connectionStringComboBox = new QComboBox(this);
    connectionStringComboBox->addItem("(Select a connection...)");
    connectionStringComboBox->addItems(connectionStrings);

    mainLayout->addWidget(helperLabel);
    mainLayout->addWidget(connectionStringComboBox);

    connect(connectionStringComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onConnectionStringSelected(int)));

    // Message text edit
    QLabel* messageLabel = new QLabel("Broadcast Message:", this);
    messageTextEdit = new QTextEdit(this);
    messageTextEdit->setPlaceholderText("Enter message to broadcast (e.g., tcp+aln://192.168.1.5:8081)");
    messageTextEdit->setMaximumHeight(100);

    mainLayout->addWidget(messageLabel);
    mainLayout->addWidget(messageTextEdit);

    // Error label (hidden by default)
    errorLabel = new QLabel(this);
    errorLabel->setStyleSheet("QLabel { color: red; }");
    errorLabel->hide();
    mainLayout->addWidget(errorLabel);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("OK", this);
    QPushButton* cancelButton = new QPushButton("Cancel", this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(okButton, SIGNAL(clicked()), this, SLOT(onOkClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(onCancelClicked()));

    setLayout(mainLayout);
    resize(500, 280);
}

int StartAdvertisementDialog::broadcastPeriodMs() const
{
    return periodSpinBox->value() * 1000;
}

QString StartAdvertisementDialog::broadcastAddress() const
{
    return broadcastAddressLineEdit->text().trimmed();
}

int StartAdvertisementDialog::broadcastPort() const
{
    return broadcastPortSpinBox->value();
}

QString StartAdvertisementDialog::message() const
{
    return messageTextEdit->toPlainText().trimmed();
}

void StartAdvertisementDialog::onConnectionStringSelected(int index)
{
    if (index > 0 && index <= availableConnectionStrings.size()) {
        messageTextEdit->setPlainText(availableConnectionStrings.at(index - 1));
    }
}

void StartAdvertisementDialog::onOkClicked()
{
    QString msg = message();
    if (msg.isEmpty()) {
        errorLabel->setText("Message cannot be empty");
        errorLabel->show();
        return;
    }

    QString addr = broadcastAddress();
    if (addr.isEmpty()) {
        errorLabel->setText("Broadcast address cannot be empty");
        errorLabel->show();
        return;
    }

    accept();
}

void StartAdvertisementDialog::onCancelClicked()
{
    reject();
}
