#include "openportdialog.h"

OpenPortDialog::OpenPortDialog(QList<QString> interfaces, QWidget *parent)
    : QDialog(parent), interfaceAddresses(interfaces)
{
    setWindowTitle("Open Port for Listening");
    setModal(true);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Horizontal layout for Host Interface and Port controls
    QHBoxLayout* topControlsLayout = new QHBoxLayout();

    // Host Interface
    QVBoxLayout* interfaceLayout = new QVBoxLayout();
    QLabel* interfaceLabel = new QLabel("Host Interface:", this);
    interfaceComboBox = new QComboBox(this);
    interfaceComboBox->addItems(interfaces);
    interfaceComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    interfaceLayout->addWidget(interfaceLabel);
    interfaceLayout->addWidget(interfaceComboBox);
    interfaceLayout->setAlignment(Qt::AlignTop);
    topControlsLayout->addLayout(interfaceLayout);

    // Port
    QVBoxLayout* portLayout = new QVBoxLayout();
    QLabel* portLabel = new QLabel("Port:", this);
    portSpinBox = new QSpinBox(this);
    portSpinBox->setMinimum(1024);
    portSpinBox->setMaximum(65535);
    portSpinBox->setValue(8081);
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

QString OpenPortDialog::selectedInterface() const
{
    return interfaceComboBox->currentText();
}

int OpenPortDialog::selectedPort() const
{
    return portSpinBox->value();
}

void OpenPortDialog::onOkClicked()
{
    if (interfaceComboBox->currentIndex() < 0) {
        errorLabel->setText("Please select a network interface");
        errorLabel->show();
        return;
    }

    accept();
}

void OpenPortDialog::onCancelClicked()
{
    reject();
}
