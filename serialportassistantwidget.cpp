#include "serialportassistantwidget.h"
#include "./ui_serialportassistantwidget.h"
#include "appcolors.h"

#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QTextBlock>
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

    // 定时发送定时框中只能输入数字
    QIntValidator* validator = new QIntValidator(1, 999999, this); 	// 最小值为 1
    ui->lineEdit_ms_times->setValidator(validator);

    // 创建并启动定时器，每 500 毫秒检测一次串口变化
    serialPortCheckTimer = new QTimer(this);
    connect(serialPortCheckTimer, &QTimer::timeout, this, &SerialPortAssistantWidget::checkSerialPorts);
    serialPortCheckTimer->start(500);

    // 创建并启动定时器，每秒更新一次时间显示
    updateRealDateTimeTimer = new QTimer(this);
    connect(updateRealDateTimeTimer, &QTimer::timeout, this, [this]() {
        ui->label_statusBar_dateTime->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    });
    // 定时器启动前就获取一次时间
    ui->label_statusBar_dateTime->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    updateRealDateTimeTimer->start(1000);

    // 多文本发送区
    for (int i = 1; i <= 9; ++i) {
        // 每项复选框集合
        auto *cb = findChild<QCheckBox*>(QString("checkBox_%1").arg(i));
        if (cb) {
            cb->setProperty("itemId", i);
            multiTextCheckBoxes.append(cb);
        }
    }
    // 多文本区域信号与槽的连接
    for (auto cb : std::as_const(multiTextCheckBoxes)) {
        int itemId = cb->property("itemId").toInt();
        auto *btnSend = findChild<QPushButton*>(QString("btn_send_%1").arg(itemId));
        auto *le = findChild<QLineEdit*>(QString("lineEdit_%1").arg(itemId));
        auto *cbHex = findChild<QCheckBox*>(QString("checkBox_hex_%1").arg(itemId));

        // 所有子复选框 -> 更新父复选框状态 信号与槽的连接
        connect(cb, &QCheckBox::checkStateChanged, this, &SerialPortAssistantWidget::updateCheckBoxAllState);

        // 发送按钮信号与槽的连接
        connect(btnSend, &QPushButton::clicked, this, [=]() {
            transmitData(le, cbHex->isChecked());
        });

        // 多文本区域输入框检测信号与槽的连接
        connect(le, &QLineEdit::textChanged, this, [=](const QString &text) {
            sendLineEditChanged(text, cbHex->isChecked());
        });

        // 多文本区域十六进制模式改变信号与槽的连接
        connect(cbHex, &QCheckBox::checkStateChanged, this, [=](Qt::CheckState state) {
            transmitHex(state, le);
        });

        // 勾选框或内容变化时更新队列
        connect(cb, &QCheckBox::checkStateChanged, this, &SerialPortAssistantWidget::updateSendQueue);
        connect(le, &QLineEdit::textChanged, this, &SerialPortAssistantWidget::updateSendQueue);
    }
    // 多文本区循环发送信号与槽的绑定
    connect(ui->checkBox_cyclicSend, &QCheckBox::checkStateChanged, this,
            &SerialPortAssistantWidget::transmitCyclically);
    // 多文本循环发送定时器
    multiTextTimer = new QTimer(this);
    multiTextTimer->setTimerType(Qt::PreciseTimer);
    connect(multiTextTimer, &QTimer::timeout, this, &SerialPortAssistantWidget::timeoutMultiTextTimer);

    // 创建发送定时器
    transmissionTimer = new QTimer(this);
    transmissionTimer->setTimerType(Qt::PreciseTimer);
    connect(transmissionTimer, &QTimer::timeout, this, [this]() {
        transmitData(ui->lineEdit_transmitString, ui->checkBox_HEXSend->isChecked());
    });

    connect(ui->btn_switch, &QPushButton::clicked, this, &SerialPortAssistantWidget::switchSerialPort);
    connect(ui->btn_transmit, &QPushButton::clicked, this, [this]() {
        transmitData(ui->lineEdit_transmitString, ui->checkBox_HEXSend->isChecked());
    });
    connect(serialPort, &QSerialPort::readyRead, this, &SerialPortAssistantWidget::readyReadSerialPort);
    connect(ui->checkBox_timedSend, &QCheckBox::checkStateChanged,
            this, &SerialPortAssistantWidget::timedTrasmission);
    connect(ui->btn_clearReceive, &QPushButton::clicked,
            this, &SerialPortAssistantWidget::clearReceivedTextEdit);
    connect(ui->btn_clearRecord, &QPushButton::clicked, this, &SerialPortAssistantWidget::clearRecordedTextEdit);
    connect(ui->btn_saveReceive, &QPushButton::clicked, this, &SerialPortAssistantWidget::saveReceivedContent);
    connect(ui->checkBox_HEX_display, &QCheckBox::checkStateChanged,
            this, &SerialPortAssistantWidget::displayHEX);
    connect(ui->checkBox_HEXSend, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        transmitHex(state, ui->lineEdit_transmitString);
    });
    connect(ui->lineEdit_transmitString, &QLineEdit::textChanged, this, [this](const QString &text) {
        sendLineEditChanged(text, ui->checkBox_HEXSend->isChecked());
    });
    connect(ui->btn_hiddenHistory, &QPushButton::clicked, this, &SerialPortAssistantWidget::hiddenHistory);
    connect(ui->btn_hiddenPanel, &QPushButton::clicked, this, &SerialPortAssistantWidget::hiddenPanel);
    connect(ui->checkBox_all, &QCheckBox::checkStateChanged,
            this, &SerialPortAssistantWidget::changeMultiTextCheckBoxAll);
}

SerialPortAssistantWidget::~SerialPortAssistantWidget()
{
    if (serialPortCheckTimer) {
        serialPortCheckTimer->stop();
        delete serialPortCheckTimer;
    }
    if (transmissionTimer) {
        transmissionTimer->stop();
        delete transmissionTimer;
    }
    if (updateRealDateTimeTimer) {
        updateRealDateTimeTimer->stop();
        delete updateRealDateTimeTimer;
    }
    if (multiTextTimer) {
        multiTextTimer->stop();
        delete multiTextTimer;
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
 * @brief SerialPortAssistantWidget::popupSerialPortDisconnect
 * 串口开启状态时，串口被拔出或故障时的弹窗提示
 */
void SerialPortAssistantWidget::popupSerialPortDisconnect()
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle(tr("错误"));
    msgBox.setText(tr("串口失效！\n请检查指定串口是否存在或完好无损"));
    msgBox.addButton(tr("确定"), QMessageBox::AcceptRole);
    msgBox.exec();
}

/**
 * @brief SerialPortAssistantWidget::convertTextEditHex
 * 根据指定的时间戳格式，将 QTextEdit 中的每条记录在 HEX 和原始文本显示之间互相转换。
 *
 * 该函数会逐行（以 QTextBlock 为单位）解析文本框内容：
 * - 当 toHex 为 true 时，将每条记录中的数据部分转换为十六进制字符串显示；
 * - 当 toHex 为 false 时，将十六进制字符串还原为原始文本。
 *
 * 转换时保留时间戳（例如 [hh:mm:ss.zzz]），
 * 并根据不同的文本框（接收区或发送记录区）使用对应的显示颜色。
 * 每条记录在最终 HTML 中以 <div> 块形式显示，确保换行与段落结构正确。
 *
 * @param textEdit          要转换的 QTextEdit 控件（如接收区或记录区）
 * @param timestampPattern  用于匹配时间戳的正则表达式（如 "\\[\\d{2}:\\d{2}:\\d{2}\\.\\d{3}\\]\\s"）
 * @param toHex             若为 true，将文本转换为 HEX 显示；若为 false，从 HEX 转回原文
 */
void SerialPortAssistantWidget::convertTextEditHex(QTextEdit* textEdit,
                                                   const QString& timestampPattern,
                                                   bool toHex)
{
    QTextDocument* doc = textEdit->document();
    QRegularExpression regex(timestampPattern);

    QString newHtml;
    // 遍历每个文档 block（每个 append() 对应一个 block）
    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        QString blockText = block.text();
        if (blockText.trimmed().isEmpty()) continue;

        QRegularExpressionMatch match = regex.match(blockText);
        if (match.hasMatch()) {
            int timestampEnd = match.capturedEnd();
            QString timestamp = blockText.left(timestampEnd);
            QString data = blockText.mid(timestampEnd);

            QString convertedData;
            if (toHex) {
                // 转为 HEX：先 trim 左右空白，再转为 bytes，然后 toHex（中间以空格分隔）
                QByteArray dataBytes = data.trimmed().toUtf8();
                convertedData = QString::fromUtf8(dataBytes.toHex(' ').toUpper());
            } else {
                // 从 HEX 转回：移除所有非 0-9A-Fa-f，然后 fromHex
                QString hexString = data;
                static const QRegularExpression nonHexPattern("[^0-9A-Fa-f]");
                hexString.remove(nonHexPattern);
                QByteArray dataBytes = QByteArray::fromHex(hexString.toLatin1());
                convertedData = QString::fromUtf8(dataBytes);
                // 如果原来是可见文本行，保持一条记录的末尾没有额外空格或换行（HTML 用 div 分隔）
            }

            // 根据不同区域使用不同颜色
            QString color = (textEdit == ui->textEditReceive) ? AppColors::ReceivedTime.name()
                                                              : AppColors::RecordTime.name();

            // 用 div 保证每条记录是独立的块，避免合并到同一行
            // 注意对 timestamp 做 HTML 转义以防止特殊字符破坏 HTML
            QString escapedTimestamp = timestamp.toHtmlEscaped();
            QString escapedData = convertedData.toHtmlEscaped();

            newHtml += QString("<div><span style=\"color:%1;\">%2</span>%3</div>")
                           .arg(color, escapedTimestamp, escapedData);
        } else {
            // 如果没匹配到时间戳就直接转义并作为独立段落插入
            newHtml += QString("<div>%1</div>").arg(blockText.toHtmlEscaped());
        }
    }

    textEdit->setHtml(newHtml);
}

/**
 * @brief SerialPortAssistantWidget::displayFrame
 * 显示接收数据
 * @param frameData 解析后的帧数据
 */
void SerialPortAssistantWidget::displayFrame(const QByteArray &frameData)
{
    if (frameData.isEmpty()) return;

    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss.zzz]");
    QString displayData;

    if (ui->checkBox_HEX_display->isChecked()) {
        displayData = QString::fromUtf8(frameData.toHex(' ').toUpper());
    } else {
        displayData = QString::fromUtf8(frameData);
    }

    QString formattedText = QString("<span style=\"color:%1;\">%2 </span>%3")
                                .arg(AppColors::ReceivedTime.name(), timestamp, displayData);

    ui->textEditReceive->append(formattedText);

    receivedBytesTotal += frameData.size();
    ui->label_statusBar_received->setText(tr("已接收: %1 字节").arg(receivedBytesTotal));
}

/**
 * @brief SerialPortAssistantWidget::multiTextItemEnable
 * 多文本发送区项使能控制
 * @param id 项ID
 * @param enable 使能控制
 */
void SerialPortAssistantWidget::multiTextItemEnable(int id, bool enable)
{
    QString btnName = QString("btn_send_%1").arg(id);
    QString leName = QString("lineEdit_%1").arg(id);
    QString hexName = QString("checkBox_hex_%1").arg(id);
    findChild<QPushButton*>(btnName)->setEnabled(enable);
    findChild<QLineEdit*>(leName)->setEnabled(enable);
    findChild<QCheckBox*>(hexName)->setEnabled(enable);
}

/*****************************************************************************************************
 * 槽函数的定义
 *****************************************************************************************************/

/**
 * @brief SerialPortAssistantWidget::switchSerialPort
 * 串口开关按键的执行逻辑
 */
void SerialPortAssistantWidget::switchSerialPort()
{
    if (serialPort->isOpen()) {
        serialPort->close();
        ui->checkBox_timedSend->setCheckState(Qt::Unchecked); 	// 关闭定时发送
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
            if (serialPort->isOpen()) { 	// 如果串口是开启的，则调用开关串口函数并弹窗提示
                // 关闭自动发送
                ui->checkBox_timedSend->setCheckState(Qt::Unchecked);
                switchSerialPort();
                popupSerialPortDisconnect();
            }
        }
        // 如果之前选中的串口仍然存在，则重新选中它
        else if (currentPortList.contains(currentSelectedPort)) {
            ui->comboBox_serialPort->setCurrentText(currentSelectedPort);
            ui->btn_switch->setDisabled(false);
        } else {
            ui->btn_switch->setDisabled(false);
            if (serialPort->isOpen()) { 	// 如果串口是开启的，则调用开关串口函数并弹窗提示
                // 关闭自动发送
                ui->checkBox_timedSend->setCheckState(Qt::Unchecked);
                switchSerialPort();
                popupSerialPortDisconnect();
            }
        }

        // 更新记录的上次串口列表
        previousPortList = currentPortList;
    }
}

/**
 * @brief SerialPortAssistantWidget::transmitData
 * 发送数据
 * @param lineEdit 指定发送内容区
 * @param hexMode 是否以十六进制模式发送
 */
void SerialPortAssistantWidget::transmitData(const QLineEdit* lineEdit, bool hexMode)
{
    if (!serialPort->isOpen()) {
        switchSerialPort();
    }

    QString text = lineEdit->text();
    if (text.isEmpty()) {
        ui->label_statusBar_sendStatus->setStyleSheet(
            QString("color: %1; font-weight: bold;").arg(AppColors::Warning.name()));
        ui->label_statusBar_sendStatus->setText(tr("没有可发送的数据！"));
        return;
    }

    QByteArray sendData;

    if (hexMode) {
        // 去除空格等非HEX字符，仅保留 0-9 A-F a-f
        QString hexString = text;
        static const QRegularExpression hexRegex("[^0-9A-Fa-f]");
        hexString.remove(hexRegex);

        // 如果字符个数为奇数，说明格式不完整，提示用户
        if (hexString.length() % 2 != 0) {
            ui->label_statusBar_sendStatus->setStyleSheet(
                QString("color: %1; font-weight: bold;").arg(AppColors::Failing.name()));
            ui->label_statusBar_sendStatus->setText(tr("HEX格式错误：长度必须为偶数"));
            return;
        }

        sendData = QByteArray::fromHex(hexString.toLatin1());
    } else {
        // 原始字符串发送
        sendData = text.toUtf8();
    }

    // 如果 “发送新行” 复选框勾选, 则需要在发送文本末尾添加换行符
    if (ui->checkBox_sendNewLine->isChecked()) {
        sendData.append("\r\n");
    }

    // 添加结束符 ETX (0x03)
    if (ui->checkBox_sendEnd->isChecked()) {
        sendData.append(char(0x03));
    }

    qint64 transmitBytes = serialPort->write(sendData);
    if (transmitBytes == -1) {
        ui->label_statusBar_sendStatus->setStyleSheet(
            QString("color: %1; font-weight: bold;").arg(AppColors::Failing.name()));
        ui->label_statusBar_sendStatus->setText(tr("发送失败！"));
        return;
    }

    // 获取时间戳
    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss.zzz]");

    // 决定 record 区显示的格式（实时对应 HEX 显示复选框）
    bool hexDisplay = ui->checkBox_HEX_display->isChecked();
    QString displayData;

    if (ui->checkBox_sendEnd->isChecked()) transmitBytes--;

    if (hexDisplay) {
        QByteArray hex = sendData.left(transmitBytes).toHex(' ').toUpper();
        displayData = QString::fromUtf8(hex);
    } else {
        // 即使以HEX发送，也允许显示为原始（可能为不可见字符）
        displayData = QString::fromUtf8(sendData.left(transmitBytes));
    }

    QString formattedText = QString("<span style=\"color:%1;\">%2 </span>%3")
                                .arg(AppColors::RecordTime.name(), timestamp, displayData);
    ui->textEditRecord->append(formattedText);

    transmittedBytesTotal += transmitBytes;
    ui->label_statusBar_sendStatus->setStyleSheet(
        QString("color: %1; font-weight: bold;").arg(AppColors::Success.name()));
    ui->label_statusBar_sendStatus->setText(tr("发送成功！"));
    ui->label_statusBar_sent->setText(tr("已发送: %1 字节").arg(transmittedBytesTotal));
}

/**
 * @brief SerialPortAssistantWidget::readyReadSerialPort
 * 接收数据，根据 “HEX显示” 复选框状态实时决定显示为 HEX 还是原文
 */
void SerialPortAssistantWidget::readyReadSerialPort()
{
    // 先读取所有数据放入缓冲
    receiveBuffer.append(serialPort->readAll());

    // 如果用户勾选“带结束符解析”
    if (ui->checkBox_receiveEnd->isChecked()) {
        int endIndex = receiveBuffer.indexOf(char(0x03));  // 查找结束符
        if (endIndex == -1) {
            return; // 没接收完一帧，等下次进来
        }

        // 取出一帧数据，去掉结束符或保留，由你决定：
        QByteArray frameData = receiveBuffer.left(endIndex); // 不包含 0x03
        receiveBuffer.remove(0, endIndex + 1);               // 清除含结束符

        displayFrame(frameData);
    }
    else {
        // 不使用结束符模式 → 所有数据直接显示，不截断，不丢弃
        if (!receiveBuffer.isEmpty()) {
            QByteArray frameData = receiveBuffer;
            receiveBuffer.clear();
            displayFrame(frameData);
        }
    }
}

/**
 * @brief SerialPortAssistantWidget::timedTrasmission
 * 定时发送数据
 */
void SerialPortAssistantWidget::timedTrasmission(Qt::CheckState state)
{
    if (state == Qt::Checked) {
        if (!serialPort->isOpen()) { 	// 如果串口没有打开则打开串口
            switchSerialPort();
        }
        ui->lineEdit_ms_times->setEnabled(false); 	// 定时框禁止修改
        ui->lineEdit_transmitString->setEnabled(false); 	// 发送信息输入框禁止修改
        ui->btn_transmit->setEnabled(false); 	// 发送按钮禁止使用
        // 开启定时器
        transmissionTimer->start(ui->lineEdit_ms_times->text().toInt());
        transmitData(ui->lineEdit_transmitString, ui->checkBox_HEXSend->isChecked()); 	// 开启定时器后立刻执行一次发送
    } else if (state == Qt::Unchecked) {
        ui->lineEdit_ms_times->setEnabled(true); 	// 定时框可以修改
        ui->lineEdit_transmitString->setEnabled(true); 	// 发送信息输入框可以修改
        ui->btn_transmit->setEnabled(true); 	// 发送按钮可以使用
        // 关闭定时器
        transmissionTimer->stop();
    }
}

/**
 * @brief SerialPortAssistantWidget::clearReceivedTextEdit
 * 清空接收区
 */
void SerialPortAssistantWidget::clearReceivedTextEdit()
{
    ui->textEditReceive->clear(); 	// 清空接收区
    receivedBytesTotal = 0;
    ui->label_statusBar_received->setText(tr("已接收: %1 字节").arg(receivedBytesTotal));
}

/**
 * @brief SerialPortAssistantWidget::clearRecordedTextEdit
 * 清空历史记录区
 */
void SerialPortAssistantWidget::clearRecordedTextEdit()
{
    ui->textEditRecord->clear();
    transmittedBytesTotal = 0;
    ui->label_statusBar_sent->setText(tr("已发送: %1 字节").arg(transmittedBytesTotal));
}

/**
 * @brief SerialPortAssistantWidget::saveReceivedContent
 * 保存接收区内容到文件
 */
void SerialPortAssistantWidget::saveReceivedContent()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("保存接收数据"), "./", tr("文本 (*.txt *.md)"));

    if (!fileName.isNull() && !fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qFatal() << "文件打开失败";
            return;
        }
        QTextStream out(&file);
        out << ui->textEditReceive->toPlainText();
        file.close();
    }
}

/**
 * @brief SerialPortAssistantWidget::displayHEX
 * 在接收区和发送区以十六进制显示
 * @param state 复选框状态
 */
void SerialPortAssistantWidget::displayHEX(Qt::CheckState state)
{
    if (state == Qt::Checked) {
        // 接收区模式：匹配时间戳格式 [hh:mm:ss.zzz]
        QString receivePattern = R"(\[\d{2}:\d{2}:\d{2}\.\d{3}\]\s)";
        // 记录区模式：匹配时间戳格式 [hh:mm:ss.zzz]
        QString recordPattern = R"(\[\d{2}:\d{2}:\d{2}\.\d{3}\]\s)";

        convertTextEditHex(ui->textEditReceive, receivePattern, true);
        convertTextEditHex(ui->textEditRecord, recordPattern, true);

    } else if (state == Qt::Unchecked) {
        // 接收区模式：匹配时间戳格式 [hh:mm:ss.zzz]
        QString receivePattern = R"(\[\d{2}:\d{2}:\d{2}\.\d{3}\]\s)";
        // 记录区模式：匹配时间戳格式 [hh:mm:ss.zzz]
        QString recordPattern = R"(\[\d{2}:\d{2}:\d{2}\.\d{3}\]\s)";

        convertTextEditHex(ui->textEditReceive, receivePattern, false);
        convertTextEditHex(ui->textEditRecord, recordPattern, false);
    }
}

/**
 * @brief SerialPortAssistantWidget::transmitHex
 * HEX发送复选框状态变化槽函数
 * @param state HEX发送复选框状态
 * @param le 对应行编辑器
 */
void SerialPortAssistantWidget::transmitHex(Qt::CheckState state, QLineEdit *le)
{
    if (state == Qt::Checked) {
        le->setPlaceholderText(tr("请输入HEX，如: 31 32 33 或 313233"));
    } else {
        le->setPlaceholderText(tr("请输入文本数据"));
        le->setStyleSheet(""); // 清除红色警告边框
    }
}

/**
 * @brief SerialPortAssistantWidget::sendLineEditChanged
 * 输入框合法性检测
 * @param text 输入框内容
 * @param hexMode 是否启用十六进制模式
 */
void SerialPortAssistantWidget::sendLineEditChanged(const QString &text, bool hexMode)
{
    // 获取触发该槽函数的对象
    QLineEdit *le = qobject_cast<QLineEdit*>(sender());

    if (ui->lineEdit_transmitString->text() == "") {
        ui->btn_transmit->setEnabled(false);
        ui->checkBox_timedSend->setEnabled(false);
    } else {
        ui->btn_transmit->setEnabled(true);
        ui->checkBox_timedSend->setEnabled(true);
    }
    if (!hexMode) return; // 普通文本模式下不处理

    QString clean = text;
    // 只保留 0-9 A-F a-f
    static const QRegularExpression hexRegex("[^0-9A-Fa-f]");
    clean.remove(hexRegex);

    // 自动每两个字符插入空格
    QString formatted;
    for (int i = 0; i < clean.length(); i += 2) {
        if (i + 2 <= clean.length())
            formatted += clean.mid(i, 2) + " ";
        else
            formatted += clean.mid(i);
    }
    formatted = formatted.trimmed();

    // 防止光标跳动：只有 text 不同时才回填
    if (formatted != text) {
        le->blockSignals(true);
        le->setText(formatted);
        le->blockSignals(false);
    }

    // 检查 HEX 长度是否为偶数
    if (clean.length() % 2 != 0) {
        le->setStyleSheet("border: 2px solid red;");
        ui->label_statusBar_sendStatus->setStyleSheet(
            QString("color: %1; font-weight: bold;").arg(AppColors::Failing.name()));
        ui->label_statusBar_sendStatus->setText(tr("HEX格式错误：必须为偶数位"));
    } else {
        le->setStyleSheet("");
        ui->label_statusBar_sendStatus->setStyleSheet(
            QString("color: %1;").arg(AppColors::FontUI.name()));
        ui->label_statusBar_sendStatus->setText(tr("状态"));
    }
}

/**
 * @brief SerialPortAssistantWidget::hiddenHistory
 * 隐藏或显示历史记录区域
 * @param checked 若为 true，则隐藏历史记录区域；若为 false 则显示历史记录区域
 */
void SerialPortAssistantWidget::hiddenHistory(bool checked)
{
    if (checked) {
        ui->groupBoxRecord->hide();
        ui->btn_hiddenHistory->setText(tr("显示历史"));
    } else {
        ui->groupBoxRecord->show();
        ui->btn_hiddenHistory->setText(tr("隐藏历史"));
    }
}

/**
 * @brief SerialPortAssistantWidget::hiddenPanel
 * 隐藏或显示面板区域
 * @param checked 若为 true，则隐藏面板；若为 false，则显示面板
 */
void SerialPortAssistantWidget::hiddenPanel(bool checked)
{
    if (checked) {
        ui->groupBoxMultiText->hide();
        ui->btn_hiddenPanel->setText(tr("显示面板"));
    } else {
        ui->groupBoxMultiText->show();
        ui->btn_hiddenPanel->setText(tr("隐藏面板"));
    }
}

/**
 * @brief SerialPortAssistantWidget::changeMultiTextCheckBoxAll
 * checkBox_all 父复选框控制子复选框
 * @param state checkBox_all 复选框状态
 */
void SerialPortAssistantWidget::changeMultiTextCheckBoxAll(Qt::CheckState state)
{
    // 防止递归调用
    bool blocked = ui->checkBox_all->blockSignals(true);

    if (!(state == Qt::PartiallyChecked)) {
        for (auto cb : std::as_const(multiTextCheckBoxes)) {
            cb->setCheckState(state);
        }
    } else { // 如果部分选中，则立刻转为全选中
        ui->checkBox_all->setCheckState(Qt::Checked);
        for (auto cb : std::as_const(multiTextCheckBoxes)) {
            cb->setCheckState(Qt::Checked);
        }
    }

    // 解除信号阻塞
    ui->checkBox_all->blockSignals(blocked);
}

/**
 * @brief SerialPortAssistantWidget::updateCheckBoxAllState
 * 根据子复选框状态更新父复选框状态
 */
void SerialPortAssistantWidget::updateCheckBoxAllState()
{
    int checkedCount = 0;

    for (auto cb : std::as_const(multiTextCheckBoxes)) {
        int itemId = cb->property("itemId").toInt();
        if (cb->isChecked()) { 	// 对应项被选中
            checkedCount++; // 记录选中的数量
            // 实现对应项组件的可交互使能
            multiTextItemEnable(itemId, true);
        } else { 	// 对应项未选中
            // 禁止对应项使能
            multiTextItemEnable(itemId, false);
        }
    }

    bool blocked = ui->checkBox_all->blockSignals(true);

    if (checkedCount == 0) {
        ui->checkBox_all->setCheckState(Qt::Unchecked);
        ui->checkBox_cyclicSend->setEnabled(false); 	// 禁止循环发送
    } else if (checkedCount == multiTextCheckBoxes.size()) {
        ui->checkBox_all->setCheckState(Qt::Checked);
        ui->checkBox_cyclicSend->setEnabled(true); 	// 使能循环发送
    } else {
        ui->checkBox_all->setCheckState(Qt::PartiallyChecked);
        ui->checkBox_cyclicSend->setEnabled(true); 	// 使能循环发送
    }

    ui->checkBox_all->blockSignals(blocked);
}

/**
 * @brief SerialPortAssistantWidget::transmitCyclically
 * 是否开启循环发送
 * @param state 循环发送复选框状态
 */
void SerialPortAssistantWidget::transmitCyclically(Qt::CheckState state)
{
    if (state == Qt::Checked) {
        // 开始循环发送，并立即发送一次
        timeoutMultiTextTimer();
        multiTextTimer->start(ui->spinBox_interval_ms->value());
    } else {
        // 停止循环发送
        multiTextTimer->stop();
        currentSendIndex = 0;
        if (lastSendId != 0) {
            findChild<QPushButton*>(QString("btn_send_%1").arg(lastSendId))->setStyleSheet("");
            lastSendId = 0;
        }
    }
}

/**
 * @brief SerialPortAssistantWidget::timeoutMultiTextTimer
 * 定时器超时触发，用于循环发送多文本区域项
 */
void SerialPortAssistantWidget::timeoutMultiTextTimer()
{
    // 没有可发送项 -> 停止定时器
    if (sendQueue.isEmpty()) {
        multiTextTimer->stop();
        ui->checkBox_cyclicSend->setCheckState(Qt::Unchecked);
        currentSendIndex = 0;
        return;
    }

    // 获取当前要发送的 itemId
    int itemId = sendQueue[currentSendIndex];

    // 触发对应 itemId 发送按钮
    if (auto *btn = findChild<QPushButton*>(QString("btn_send_%1").arg(itemId))) {
        // 使上一次发送按钮恢复正常
        if (lastSendId != 0) {
            findChild<QPushButton*>(QString("btn_send_%1").arg(lastSendId))->setStyleSheet("");
        }
        // 高亮当前发送按钮
        btn->setStyleSheet(QString("border: 2px solid %1; border-radius: 6px; background-color: %2;")
                               .arg(AppColors::highlightCyclic.name(), AppColors::bacCyclic.name()));
        // 触发对应项发送按钮
        emit btn->clicked();
        lastSendId = itemId;
    }

    // 指向下一个发送项（循环）
    currentSendIndex = (currentSendIndex + 1) % sendQueue.size();

    // 动态更新时间间隔（如果用户修改了时间间隔）
    int currentInterval = ui->spinBox_interval_ms->value();
    if (multiTextTimer->interval() != currentInterval) {
        multiTextTimer->setInterval(currentInterval);
    }
}

/**
 * @brief SerialPortAssistantWidget::updateSendQueue
 * 更新发送队列
 */
void SerialPortAssistantWidget::updateSendQueue()
{
    int currentId = 0;
    if (!sendQueue.isEmpty()) {
        currentId = sendQueue[currentSendIndex];
    }
    sendQueue.clear();

    // 将最新有效项更新到队列
    for (auto cb : std::as_const(multiTextCheckBoxes)) {
        if (cb->isChecked()) {
            int itemId = cb->property("itemId").toInt();
            auto *le = findChild<QLineEdit*>(QString("lineEdit_%1").arg(itemId));
            if (le && le->text() != "") {
                sendQueue.push_back(itemId);
            }
        }
    }

    // 如果当前正在循环发送，但有效项没了 -> 停止
    if (sendQueue.isEmpty() && multiTextTimer -> isActive()) {
        emit ui->checkBox_cyclicSend->setCheckState(Qt::Unchecked);
        return;
    }

    // 确定下一个发送项在队列中的索引
    int temp = sendQueue.indexOf(currentId);
    if (temp != -1) { 	// 找到了则更新新的索引位置
        currentSendIndex = temp;
    } else if (currentSendIndex >= sendQueue.size()) {
        // 没找到且索引越界，则从头开始
        currentSendIndex = 0;
    }
}
