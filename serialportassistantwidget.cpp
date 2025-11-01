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

    // 创建发送定时器
    transmissionTimer = new QTimer(this);
    transmissionTimer->setTimerType(Qt::PreciseTimer);
    connect(transmissionTimer, &QTimer::timeout, this, [this]() { transmitData(); });

    connect(ui->btn_switch, &QPushButton::clicked, this, &SerialPortAssistantWidget::switchSerialPort);
    connect(ui->btn_transmit, &QPushButton::clicked, this, &SerialPortAssistantWidget::transmitData);
    connect(serialPort, &QSerialPort::readyRead, this, &SerialPortAssistantWidget::readyReadSerialPort);
    connect(ui->checkBox_timedSend, &QCheckBox::checkStateChanged,
            this, &SerialPortAssistantWidget::timedTrasmission);
    connect(ui->btn_clearReceive, &QPushButton::clicked,
            this, &SerialPortAssistantWidget::clearReceivedTextEdit);
    connect(ui->btn_clearRecord, &QPushButton::clicked, this, &SerialPortAssistantWidget::clearRecordedTextEdit);
    connect(ui->btn_saveReceive, &QPushButton::clicked, this, &SerialPortAssistantWidget::saveReceivedContent);
    connect(ui->checkBox_HEX_display, &QCheckBox::checkStateChanged,
            this, &SerialPortAssistantWidget::displayHEX);
    connect(ui->checkBox_HEXSend, &QCheckBox::checkStateChanged,
            this, &SerialPortAssistantWidget::transimitHex);
    connect(ui->lineEdit_transmitString, &QLineEdit::textChanged,
            this, &SerialPortAssistantWidget::sendLineEditChanged);
    connect(ui->btn_hiddenHistory, &QPushButton::clicked, this, &SerialPortAssistantWidget::hiddenHistory);
    connect(ui->btn_hiddenPanel, &QPushButton::clicked, this, &SerialPortAssistantWidget::hiddenPanel);
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
 * 发送数据, 根据 “HEX显示” 复选框状态实时决定显示为 HEX 还是原文
 */

void SerialPortAssistantWidget::transmitData()
{
    if (!serialPort->isOpen()) {
        switchSerialPort();
    }

    QString text = ui->lineEdit_transmitString->text();
    if (text.isEmpty()) {
        ui->label_statusBar_sendStatus->setStyleSheet("color: orange; font-weight: bold;");
        ui->label_statusBar_sendStatus->setText(tr("没有可发送的数据！"));
        return;
    }

    QByteArray sendData;
    bool hexSendMode = ui->checkBox_HEXSend->isChecked();     // ✅ 新增：勾选则按HEX发送

    if (hexSendMode) {
        // 去除空格等非HEX字符，仅保留 0-9 A-F a-f
        QString hexString = text;
        static const QRegularExpression hexRegex("[^0-9A-Fa-f]");
        hexString.remove(hexRegex);

        // 如果字符个数为奇数，说明格式不完整，提示用户
        if (hexString.length() % 2 != 0) {
            ui->label_statusBar_sendStatus->setStyleSheet("color: red; font-weight: bold;");
            ui->label_statusBar_sendStatus->setText(tr("HEX格式错误：长度必须为偶数"));
            return;
        }

        sendData = QByteArray::fromHex(hexString.toLatin1());
    } else {
        // 原始字符串发送
        sendData = text.toUtf8();
    }

    // 如果 “发送新行” 复选框勾选, 则需要在发送文本末尾添加换行符
    if (ui->checkBox_sendNewLine->checkState() == Qt::Checked) {
        sendData.append("\r\n");
    }

    // 添加结束符 ETX (0x03)
    sendData.append(char(0x03));

    qint64 transmitBytes = serialPort->write(sendData);
    if (transmitBytes == -1) {
        ui->label_statusBar_sendStatus->setStyleSheet("color: red; font-weight: bold;");
        ui->label_statusBar_sendStatus->setText(tr("发送失败！"));
        return;
    }

    // 获取时间戳
    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss.zzz]");

    // ✅ 决定 record 区显示的格式（实时对应 HEX 显示复选框）
    bool hexDisplay = ui->checkBox_HEX_display->isChecked();
    QString displayData;

    if (hexDisplay) {
        QByteArray hex = sendData.left(transmitBytes - 1).toHex(' ').toUpper();
        displayData = QString::fromUtf8(hex);
    } else {
        if (hexSendMode) {
            // 即使以HEX发送，也允许显示为原始（可能为不可见字符）
            displayData = QString::fromUtf8(sendData.left(transmitBytes - 1));
        } else {
            displayData = QString::fromUtf8(sendData.left(transmitBytes - 1));
        }
    }

    QString formattedText = QString("<span style=\"color:%1;\">%2 </span>%3")
                                .arg(AppColors::RecordTime.name(), timestamp, displayData);
    ui->textEditRecord->append(formattedText);

    transmittedBytesTotal += transmitBytes - 1;
    ui->label_statusBar_sendStatus->setStyleSheet("color: #00FF7F; font-weight: bold;");
    ui->label_statusBar_sendStatus->setText(tr("发送成功！"));
    ui->label_statusBar_sent->setText(tr("已发送: %1 字节").arg(transmittedBytesTotal));
}

/**
 * @brief SerialPortAssistantWidget::readyReadSerialPort
 * 接收数据，根据 “HEX显示” 复选框状态实时决定显示为 HEX 还是原文
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

        QString displayData;
        // 判断当前是否处于 HEX 显示模式
        if (ui->checkBox_HEX_display->isChecked()) {
            // 转为 HEX 格式显示
            QByteArray hexBytes = frameData.toHex(' ').toUpper();
            displayData = QString::fromUtf8(hexBytes);
        } else {
            // 按原始文本显示
            displayData = QString::fromUtf8(frameData);
        }
        // 拼接格式化文本
        QString formattedText = QString("<span style=\"color:%1;\">%2 </span>%3")
                                    .arg(AppColors::ReceivedTime.name(), timestamp, displayData);
        // 显示到接收区
        ui->textEditReceive->append(formattedText);

        int receivedLineBytes = frameData.size();
        receivedBytesTotal += receivedLineBytes;
        ui->label_statusBar_received->setText(tr("已接收: %1 字节").arg(receivedBytesTotal));
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
        transmitData(); 	// 开启定时器后立刻执行一次发送
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
}

/**
 * @brief SerialPortAssistantWidget::clearRecordedTextEdit
 * 清空历史记录区
 */
void SerialPortAssistantWidget::clearRecordedTextEdit()
{
    ui->textEditRecord->clear();
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
 * @brief SerialPortAssistantWidget::transimitHex
 * HEX发送复选框状态变化槽函数
 * @param state HEX发送复选框状态
 */
void SerialPortAssistantWidget::transimitHex(Qt::CheckState state)
{
    if (state == Qt::Checked) {
        ui->lineEdit_transmitString->setPlaceholderText(tr("请输入HEX，如: 31 32 33 或 313233"));
    } else {
        ui->lineEdit_transmitString->setPlaceholderText(tr("请输入文本数据"));
        ui->lineEdit_transmitString->setStyleSheet(""); // 清除红色警告边框
    }
}

/**
 * @brief 自动格式化 lineEdit_transmitString 输入内容（在 HEX 发送模式下）
 *        1. 过滤非法字符（只保留 0-9 A-F a-f）
 *        2. 每 2 个字符插入空格
 */
void SerialPortAssistantWidget::sendLineEditChanged(const QString &text)
{
    if (!ui->checkBox_HEXSend->isChecked())
        return; // 普通文本模式下不处理

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
        ui->lineEdit_transmitString->blockSignals(true);
        ui->lineEdit_transmitString->setText(formatted);
        ui->lineEdit_transmitString->blockSignals(false);
    }

    // 检查 HEX 长度是否为偶数
    if (clean.length() % 2 != 0) {
        ui->lineEdit_transmitString->setStyleSheet("border: 2px solid red;");
        ui->label_statusBar_sendStatus->setText("HEX格式错误：必须为偶数位");
    } else {
        ui->lineEdit_transmitString->setStyleSheet("");
        ui->label_statusBar_sendStatus->setText("");
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
