#ifndef SERIALPORTASSISTANTWIDGET_H
#define SERIALPORTASSISTANTWIDGET_H

#include <QSerialPort>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class SerialPortAssistantWidget;
}
QT_END_NAMESPACE

class SerialPortAssistantWidget : public QWidget
{
    Q_OBJECT

public:
    SerialPortAssistantWidget(QWidget *parent = nullptr);
    ~SerialPortAssistantWidget();

private:
    Ui::SerialPortAssistantWidget *ui;
    QSerialPort* serialPort;
    QHash<QString, QSerialPort::Parity> parityMap;
    QHash<QString, QSerialPort::StopBits> stopBitsMap;
    void ConfigMap();

private slots:
    void switchSerialPort();
};
#endif // SERIALPORTASSISTANTWIDGET_H
