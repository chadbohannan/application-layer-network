#ifndef TESTALN_H
#define TESTALN_H

#include <QTest>
#include <QObject>

class TestAln : public QObject
{
    Q_OBJECT
public:
    explicit TestAln(QObject *parent = nullptr);

signals:

private slots:
    void initTestCase();
    void parsePacket();
};

#endif // TESTALN_H
