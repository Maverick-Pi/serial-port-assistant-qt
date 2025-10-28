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
    int transmittedBytesTotal; 	// 发送数据字节总数
    int receivedBytesTotal; 	// 接收数据字节总数
    QByteArray receiveBuffer; 	// 接收数据缓冲区
    QTimer* transmissionTimer; 	// 发送定时器
    QTimer* updateRealDateTimeTimer; 	// 实时时间更新定时器

    void configMap(); 	// 串口配置映射
    void updateSerialPortList(); 	// 更新串口列表
    void enabledSerialPortConfig(bool en); 	// 串口配置选项使能控制
    void popupSerialPortDisconnect(); 	// 串口打开时，串口被拔出发出的提示

private slots:
    void switchSerialPort(); 	// 开关串口
    void checkSerialPorts(); 	// 定时检测串口
    void transmitData(); 	// 发送数据
    void readyReadSerialPort(); 	// 读取接收数据
    void timedTrasmission(Qt::CheckState state); 	// 定时发送
    void clearReceivedTextEdit(); 	// 清空接收区
    void saveReceivedContent(); 	// 保存接收区内容
    void displayHEX(Qt::CheckState state); 	// hex 显示
};
#endif // SERIALPORTASSISTANTWIDGET_H
