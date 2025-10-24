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
    QSerialPort* serialPort; 	// 串口实例
    QHash<QString, QSerialPort::Parity> parityMap; 	// 串口校验位选项映射
    QHash<QString, QSerialPort::StopBits> stopBitsMap; 	// 串口停止位选项映射
    QTimer* serialPortCheckTimer; 	// 串口检测定时器
    QStringList previousPortList; 	// 上一次检测到的串口列表

    void configMap(); 	// 串口配置映射
    void updateSerialPortList(); 	// 更新串口列表

private slots:
    void switchSerialPort();
    void checkSerialPorts(); 	// 定时检测串口
};
#endif // SERIALPORTASSISTANTWIDGET_H
