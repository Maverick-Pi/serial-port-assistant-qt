#ifndef SERIALPORTASSISTANTWIDGET_H
#define SERIALPORTASSISTANTWIDGET_H

#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSerialPort>
#include <QTextEdit>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class SerialPortAssistantWidget;
}
QT_END_NAMESPACE

#define SERIALPORT_RECEIVE_COLOR #

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
    QList<QCheckBox*> multiTextCheckBoxes; 	// 多文本发送区项复选框集合

    void configMap(); 	// 串口配置映射
    void updateSerialPortList(); 	// 更新串口列表
    void enabledSerialPortConfig(bool en); 	// 串口配置选项使能控制
    void popupSerialPortDisconnect(); 	// 串口打开时，串口被拔出发出的提示
    // 接收区、历史记录区文本与 HEX 显示相互转换
    void convertTextEditHex(QTextEdit* textEdit, const QString& timestampPattern, bool toHex);
    void displayFrame(const QByteArray &frameData); // 显示接收数据
    void multiTextItemEnable(int id, bool enable); 	// 多文本发送区项的使能控制

private slots:
    void switchSerialPort(); 	// 开关串口
    void checkSerialPorts(); 	// 定时检测串口
    void transmitData(const QLineEdit* lineEdit, bool hexMode); 	// 发送数据
    void readyReadSerialPort(); 	// 读取接收数据
    void timedTrasmission(Qt::CheckState state); 	// 定时发送
    void clearReceivedTextEdit(); 	// 清空接收区
    void clearRecordedTextEdit(); 	// 清空历史记录区
    void saveReceivedContent(); 	// 保存接收区内容
    void displayHEX(Qt::CheckState state); 	// hex 显示
    void transmitHex(Qt::CheckState state, QLineEdit *le); 	// HEX 发送模式
    /**
     * @brief 自动格式化 lineEdit_transmitString 输入内容（在 HEX 发送模式下）
     *        1. 过滤非法字符（只保留 0-9 A-F a-f）
     *        2. 每 2 个字符插入空格
     */
    void sendLineEditChanged(const QString &text, bool hexMode); // 对输入框进行实时监控（仅在 hexSend 模式下启用）
    void hiddenHistory(bool checked); 	// 隐藏历史记录
    void hiddenPanel(bool checked); 	// 隐藏面板
    void changeMultiTextCheckBoxAll(Qt::CheckState state); 	// 全选复选框
    void updateCheckBoxAllState(); 	// 根据子复选框状态更新父复选框三态
};
#endif // SERIALPORTASSISTANTWIDGET_H
