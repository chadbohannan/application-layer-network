#include "addchanneldialog.h"

#include <QHBoxLayout>

AddChannelDialog::AddChannelDialog(QWidget *parent) : QDialog(parent) {
    textEdit = new QPlainTextEdit;
    QTextDocument *pdoc = textEdit->document ();
    QFontMetrics fm (pdoc->defaultFont ());
    QMargins margins = textEdit->contentsMargins ();
    int nHeight = fm.lineSpacing () +
        (pdoc->documentMargin () +
        textEdit->frameWidth ()) * 2 +
        margins.top () +
        margins.bottom ();
    textEdit->setMaximumHeight(nHeight*2);
    textEdit->setPlainText("tcp+aln://localhost:8181"); // dev convenience
    errorLabel = new QLabel;
    okButton = new QPushButton(tr("Ok"));
    QHBoxLayout* bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(errorLabel);
    bottomLayout->addWidget(okButton);
    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(textEdit);
    mainLayout->addLayout(bottomLayout);

    setLayout(mainLayout);
    connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked() ) );
}

void AddChannelDialog::okClicked() {
    qDebug() << textEdit->toPlainText();
    QString txt = textEdit->toPlainText();
    QUrl url = QUrl::fromEncoded(txt.toUtf8(), QUrl::StrictMode);
    if (url.isValid() && url.toDisplayString() == txt) {
        emit connectRequest(txt);
        this->close();
    } else {
        errorLabel->setText("URL format error " + url.errorString());
    }

    qDebug() << url.scheme() << url.host() << url.port() << url.path().remove("/");
}
