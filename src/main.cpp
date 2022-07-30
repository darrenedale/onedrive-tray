#include <QApplication>
#include <iostream>

#ifndef QT_NO_SYSTEMTRAYICON

#include "Application.h"

using OneDrive::Application;

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(systray);

    try {
        Application app(argc, argv);
        return Application::exec();
    } catch (std::exception & err) {
        std::cerr << err.what() << "\n" << std::flush;
        return 1;
    }
}

#else

#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QString text("QSystemTrayIcon is not supported on this platform");

    QLabel *label = new QLabel(text);
    label->setWordWrap(true);

    label->show();
    qDebug() << text;

    app.exec();
}

#endif
