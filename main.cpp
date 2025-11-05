#include "serialportassistantwidget.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    // 设置默认语言为英语
    QString language = "en_US";
    // 检测系统语言
    for (const QString &locale : uiLanguages) {
        if (locale.startsWith("zh")) {
            language = "zh_CN";
            break;
        } else if (locale.startsWith("en")) {
            language = "en_US";
            break;
        }
    }
    // 加载对应的翻译文件
    const QString baseName = "serial-port-assistant_" + language;
    if (translator.load(":/i18n/" + baseName)) {
        a.installTranslator(&translator);
    }

    SerialPortAssistantWidget w;
    w.show();
    return a.exec();
}
