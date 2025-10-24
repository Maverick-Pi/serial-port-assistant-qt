#include "serialportassistantwidget.h"
#include "./ui_serialportassistantwidget.h"

#include <QSerialPortInfo>
#include <QTimer>

SerialPortAssistantWidget::SerialPortAssistantWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SerialPortAssistantWidget)
{
    ui->setupUi(this);

    serialPort = new QSerialPort(this);

    // 初始化映射表
    configMap();

    // 初始化串口列表
    updateSerialPortList();

    // 创建并启动定时器，每秒检测一次串口变化
    serialPortCheckTimer = new QTimer(this);
    connect(serialPortCheckTimer, &QTimer::timeout, this, &SerialPortAssistantWidget::checkSerialPorts);
    serialPortCheckTimer->start(1000);

    connect(ui->btn_switch, &QPushButton::clicked, this, &SerialPortAssistantWidget::switchSerialPort);
}

SerialPortAssistantWidget::~SerialPortAssistantWidget()
{
    if (serialPortCheckTimer) {
        serialPortCheckTimer->stop();
        delete serialPortCheckTimer;
    }
    delete ui;
    delete serialPort;
}

/**
 * @brief SerialPortAssistantWidget::configMap
 * 串口配置映射
 */
void SerialPortAssistantWidget::configMap()
{
    // 校验位枚举映射
    parityMap["None"] = QSerialPort::NoParity;
    parityMap["Even"] = QSerialPort::EvenParity;
    parityMap["Mark"] = QSerialPort::MarkParity;
    parityMap["Odd"] = QSerialPort::OddParity;
    parityMap["Space"] = QSerialPort::SpaceParity;
    // 停止位枚举映射
    stopBitsMap["One"] = QSerialPort::OneStop;
    stopBitsMap["OneAndHalf"] = QSerialPort::OneAndHalfStop;
    stopBitsMap["Two"] = QSerialPort::TwoStop;
}

/**
 * @brief SerialPortAssistantWidget::updateSerialPortList
 * 更新串口列表
 */
void SerialPortAssistantWidget::updateSerialPortList()
{
    ui->comboBox_serialPort->clear();
    previousPortList.clear();

    QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &i : std::as_const(serialPortInfoList)) {
        ui->comboBox_serialPort->addItem(i.portName());
        previousPortList.append(i.portName());
    }

    // 如果没有可用串口，可以添加一个提示项
    if (previousPortList.isEmpty()) {
        ui->btn_switch->setDisabled(true);
        ui->comboBox_serialPort->addItem("无可用串口");
    } else {
        ui->btn_switch->setDisabled(false);
    }
}

/**
 * 槽函数的定义
 */

/**
 * @brief SerialPortAssistantWidget::switchSerialPort
 * 串口开关按键的执行逻辑
 */
void SerialPortAssistantWidget::switchSerialPort()
{
    // 如果当前没有可用串口，将按钮 disable，同时直接返回
    if (previousPortList.isEmpty()) {
        return;
    }
    // 选择端口号
    serialPort->setPortName(ui->comboBox_serialPort->currentText());
    // 配置波特率
    serialPort->setBaudRate(ui->comboBox_baud->currentText().toInt());
    // 配置数据位
    serialPort->setDataBits(QSerialPort::DataBits(ui->comboBox_dataBit->currentText().toUInt()));
    // 配置校验位
    serialPort->setParity(parityMap[ui->comboBox_check->currentText()]);
    // 配置停止位
    serialPort->setStopBits(stopBitsMap[ui->comboBox_stop->currentText()]);
    // 配置流控
    serialPort->setFlowControl(QSerialPort::FlowControl(ui->comboBox_flowCtrl->currentIndex()));

    if (serialPort->open(QIODeviceBase::ReadWrite)) {
        qDebug() << "打开成功！";
        ui->btn_switch->setText(tr("关闭串口"));
    } else {
        qFatal() << "串口打开失败！";
    }
}

/**
 * @brief SerialPortAssistantWidget::checkSerialPorts
 * 检测串口
 */
void SerialPortAssistantWidget::checkSerialPorts()
{
    QStringList currentPortList;

    // 获取当前的可用串口列表
    QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &i : std::as_const(serialPortInfoList)) {
        currentPortList.append(i.portName());
    }

    // 如果串口列表发生变化
    if (currentPortList != previousPortList) {
        qDebug() << "串口列表发生变化，更新列表...";

        // 保存当前选中的串口
        QString currentSelectedPort = ui->comboBox_serialPort->currentText();

        // 更新串口列表
        ui->comboBox_serialPort->clear();

        for (const QString &portName : std::as_const(currentPortList)) {
            ui->comboBox_serialPort->addItem(portName);
        }

        // 如果没有可用接口，添加提示
        if (currentPortList.isEmpty()) {
            ui->btn_switch->setDisabled(true);
            ui->comboBox_serialPort->addItem(tr("无可用串口"));
        }
        // 如果之前选中的串口仍然存在，则重新选中它
        else if (currentPortList.contains(currentSelectedPort)) {
            ui->comboBox_serialPort->setCurrentText(currentSelectedPort);
            ui->btn_switch->setDisabled(false);
        } else {
            ui->btn_switch->setDisabled(false);
        }

        // 更新记录的上次串口列表
        previousPortList = currentPortList;
        qDebug() << "当前可用串口：" << currentPortList;
    }
}
