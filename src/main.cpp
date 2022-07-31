#include <QApplication>
#include <iostream>

#include "Application.h"

#ifdef QT_NO_SYSTEMTRAYICON
#error QSystemTrayIcon is not supported on the target platform.
#endif

using OneDrive::Application;

int main(int argc, char * argv[])
{
    Q_INIT_RESOURCE(systray);

    try {
        Application app(argc, argv);
        return app.exec();
    } catch (std::exception & err) {
        std::cerr << err.what() << "\n" << std::flush;
        return 1;
    }
}
