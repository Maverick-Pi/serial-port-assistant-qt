#include "serialportassistantwidget.h"
#include "./ui_serialportassistantwidget.h"

#include <QSerialPortInfo>

SerialPortAssistantWidget::SerialPortAssistantWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SerialPortAssistantWidget)
{
    ui->setupUi(this);

    serialPort = new QSerialPort(this);

    QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &i : std::as_const(serialPortInfoList)) {
        ui->comboBox_serialPort->addItem(i.portName());
    }

    connect(ui->btn_switch, &QPushButton::clicked, this, &SerialPortAssistantWidget::switchSerialPort);
}

SerialPortAssistantWidget::~SerialPortAssistantWidget()
{
    delete ui;
    delete serialPort;
}

void SerialPortAssistantWidget::ConfigMap()
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
 * 槽函数的定义
 */

/**
 * @brief SerialPortAssistantWidget::switchSerialPort
 * 串口开关按键的执行逻辑
 */
void SerialPortAssistantWidget::switchSerialPort()
{
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
