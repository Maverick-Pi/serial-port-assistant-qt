#include "serialportassistantwidget.h"
#include "./ui_serialportassistantwidget.h"

#include <QDateTime>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QTimer>

SerialPortAssistantWidget::SerialPortAssistantWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SerialPortAssistantWidget)
{
    ui->setupUi(this);

    serialPort = new QSerialPort(this);
    // 初始化发送数据字节总数
    transmittedBytesTotal = 0;
    // 初始化接收数据字节总数
    receivedBytesTotal = 0;

    // 初始化映射表
    configMap();

    // 初始化串口列表
    updateSerialPortList();

    // 创建并启动定时器，每秒检测一次串口变化
    serialPortCheckTimer = new QTimer(this);
    connect(serialPortCheckTimer, &QTimer::timeout, this, &SerialPortAssistantWidget::checkSerialPorts);
    serialPortCheckTimer->start(1000);

    connect(ui->btn_switch, &QPushButton::clicked, this, &SerialPortAssistantWidget::switchSerialPort);
    connect(ui->btn_transmit, &QPushButton::clicked, this, &SerialPortAssistantWidget::transmitData);
    connect(serialPort, &QSerialPort::readyRead, this, &SerialPortAssistantWidget::readyReadSerialPort);
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
 * @brief SerialPortAssistantWidget::enableSerialPortConfig
 * 串口配置选项使能
 * @param en 使能控制
 */
void SerialPortAssistantWidget::enabledSerialPortConfig(bool en)
{
    ui->comboBox_serialPort->setEnabled(en);
    ui->comboBox_baud->setEnabled(en);
    ui->comboBox_dataBit->setEnabled(en);
    ui->comboBox_check->setEnabled(en);
    ui->comboBox_stop->setEnabled(en);
    ui->comboBox_flowCtrl->setEnabled(en);
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
    if (serialPort->isOpen()) {
        serialPort->close();
        ui->btn_switch->setText(tr("打开串口"));
        enabledSerialPortConfig(true);
        return;
    }
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
        ui->btn_switch->setText(tr("关闭串口"));
        enabledSerialPortConfig(false);
    } else {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle(tr("错误"));
        msgBox.setText(tr("打开串口失败！\n请检查指定串口是否存在或者已被打开"));
        msgBox.addButton(tr("确定"), QMessageBox::AcceptRole);
        msgBox.exec();
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

/**
 * @brief SerialPortAssistantWidget::transmitData
 * 发送数据
 */
void SerialPortAssistantWidget::transmitData()
{
    if (!serialPort->isOpen()) { // 检测串口是否打开，若没有打开则打开
        switchSerialPort();
    }
    QString text = ui->lineEdit_transmitString->text();
    if (!text.isEmpty()) {
        QByteArray sendData = text.toUtf8();
        sendData.append(char(0x03)); // 添加结束符 ETX
        qint64 transmitBytes = serialPort->write(sendData);

        if (transmitBytes == -1) {
            ui->label_statusBar_sendStatus->setStyleSheet("color: red; font-weight: bold;");
            ui->label_statusBar_sendStatus->setText(tr("发送失败！"));
        } else {
            // 获取当前时间并格式化
            QDateTime currentTime = QDateTime::currentDateTime();
            QString timestamp = currentTime.toString("[hh:mm:ss.zzz]");
            // 创建带格式的文本
            QString formattedText = QString("<span style=\"color:#98FB98;\">%1 &lt;- </span>%2")
                                        .arg(timestamp, sendData.left(transmitBytes - 1));
            // 使用 HEML 格式插入历史记录文本
            ui->textEditRecord->append(formattedText);
            // 计算发送数据的字节数
            transmittedBytesTotal += transmitBytes - 1;
            // 渲染发送结果状态
            ui->label_statusBar_sendStatus->setStyleSheet("color: #00FF7F; font-weight: bold;");
            ui->label_statusBar_sendStatus->setText(tr("发送成功！"));
            ui->label_statusBar_sent->setText(tr("已发送: %1 字节").arg(transmittedBytesTotal));
        }
    } else {
        ui->label_statusBar_sendStatus->setStyleSheet("color: orange; font-weight: bold;");
        ui->label_statusBar_sendStatus->setText(tr("没有可发送的数据！"));
    }
}

/**
 * @brief SerialPortAssistantWidget::readyReadSerialPort
 * 接收数据并加入缓冲区中
 */
void SerialPortAssistantWidget::readyReadSerialPort()
{
    // 将数据添加到缓冲区
    receiveBuffer.append(serialPort->readAll());

    int endIndex = receiveBuffer.indexOf(char(0x03)); 	// 查找结束符
    if (endIndex == -1) { 	// 没有完整帧就等下一次触发
        return;
    }

    // 取出一帧数据（不包含结束符）
    QByteArray frameData = receiveBuffer.left(endIndex);
    receiveBuffer.remove(0, endIndex + 1);

    if (!frameData.isEmpty()) {
        // 获取当前时间并格式化
        QDateTime currentTime = QDateTime::currentDateTime();
        QString timestamp = currentTime.toString("[hh:mm:ss.zzz]");
        // 创建带格式的文本
        QString formattedText = QString("<span style=\"color:#87CEEB;\">%1 -&gt; </span>%2")
                                    .arg(timestamp, QString::fromUtf8(frameData));
        // 使用 HTML 格式插入文本
        ui->textEditReceive->append(formattedText);
        int receivedLineBytes = frameData.size();
        receivedBytesTotal += receivedLineBytes;
        ui->label_statusBar_received->setText(tr("已接收: %1 字节").arg(receivedBytesTotal));
    }
}
