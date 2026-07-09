#include "startlistendialog.h"

StartListenDialog::StartListenDialog(QString defaultBroadcastAddr, int defaultPort, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Start Listening for Broadcasts");
    setModal(true);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Horizontal layout for Host Interface and Port controls
    QHBoxLayout* topControlsLayout = new QHBoxLayout();

    // Host Interface
    QVBoxLayout* interfaceLayout = new QVBoxLayout();
    QLabel* interfaceLabel = new QLabel("Host Interface:", this);
    broadcastAddressLineEdit = new QLineEdit(this);
    broadcastAddressLineEdit->setText(defaultBroadcastAddr);
    broadcastAddressLineEdit->setPlaceholderText("e.g., 192.168.1.255");
    broadcastAddressLineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    interfaceLayout->addWidget(interfaceLabel);
    interfaceLayout->addWidget(broadcastAddressLineEdit);
    interfaceLayout->setAlignment(Qt::AlignTop);
    topControlsLayout->addLayout(interfaceLayout);

    // Port
    QVBoxLayout* portLayout = new QVBoxLayout();
    QLabel* portLabel = new QLabel("Port:", this);
    portSpinBox = new QSpinBox(this);
    portSpinBox->setMinimum(1024);
    portSpinBox->setMaximum(65535);
    portSpinBox->setValue(defaultPort);
    portSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    portLayout->addWidget(portLabel);
    portLayout->addWidget(portSpinBox);
    portLayout->setAlignment(Qt::AlignTop);
    topControlsLayout->addLayout(portLayout);

    mainLayout->addLayout(topControlsLayout);

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
    resize(400, 120);
}

QString StartListenDialog::broadcastAddress() const
{
    return broadcastAddressLineEdit->text().trimmed();
}

int StartListenDialog::port() const
{
    return portSpinBox->value();
}

void StartListenDialog::onOkClicked()
{
    QString addr = broadcastAddress();
    if (addr.isEmpty()) {
        errorLabel->setText("Broadcast address cannot be empty");
        errorLabel->show();
        return;
    }

    accept();
}

void StartListenDialog::onCancelClicked()
{
    reject();
}
