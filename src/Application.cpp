//
// Created by darren on 30/07/22.
//

#include "Application.h"

#include <QLatin1String>
#include <QLocale>
#include <QCommandLineParser>
#include <QSystemTrayIcon>
#include <QMessageBox>

#include "Window.h"

using namespace OneDrive;

Application::Application(int &argc, char **argv)
        : QApplication(argc, argv),
          m_oneDrivePath(),
          m_oneDriveArgs(),
          m_window(),
          m_qtTranslator(),
          m_appTranslator()
{
    setAttribute(Qt::AA_UseHighDpiPixmaps);
    setOrganizationDomain(QLatin1String("dev.equit"));
    setOrganizationName(QLatin1String("Équit"));
    setApplicationDisplayName(tr("OneDrive sync app"));
    setApplicationVersion(QLatin1String("3.0"));
    QApplication::setWindowIcon(QIcon(":window-icon"));

    // Define the command line parameters
    QCommandLineParser parser;
    parser.setApplicationDescription("Run and control OneDrive from the system tray");
    parser.addHelpOption();
    QCommandLineOption onedrivePathOption(QStringList() << "p" << "onedrive-path", "Path to the OneDrive program",
                                          "path");
    parser.addOption(onedrivePathOption);
    QCommandLineOption onedriveArgsOption(QStringList() << "a" << "onedrive-args", "Arguments passed to OneDrive",
                                          "args");
    parser.addOption(onedriveArgsOption);
    QCommandLineOption silentFailOption(QStringList() << "s" << "silent-fail",
                                        "No error message displayed when no system tray is detected");
    parser.addOption(silentFailOption);
    parser.process(*this);

    installTranslators();

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        if (!parser.isSet(silentFailOption)) {
            showNotification(tr("No system tray is available."));
        }

        throw RuntimeException("No system tray.");
    }

    m_oneDrivePath = parser.value(onedrivePathOption);
    m_oneDriveArgs = parser.value(onedriveArgsOption);

    setQuitOnLastWindowClosed(false);
    m_window = std::make_unique<Window>(m_oneDrivePath, m_oneDriveArgs);
}

Application::~Application() noexcept = default;

void Application::installTranslators()
{
    if (m_qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        installTranslator(&m_qtTranslator);
    }

    // Loads path + filename + prefix + ui language name + suffix (".qm" if the suffix is not specified)
    if (m_appTranslator.load(QLocale(), applicationName(), "_", applicationDirPath())) {
        installTranslator(&m_appTranslator);
    } else {
        qDebug().noquote() << "Translation not found for" << QLocale::languageToString(QLocale().language()) << "language" << QLocale().uiLanguages() << ".";
    }
}

const QString & Application::oneDrivePath() const
{
    return m_oneDrivePath;
}

const QString & Application::oneDriveArgs() const
{
    return m_oneDriveArgs;
}

void Application::showNotification(const QString &message, int timeout, OneDrive::Application::NotificationType type)
{
    QMessageBox::StandardButton (*messageFunction)(QWidget *, const QString &,const QString &, QMessageBox::StandardButtons, QMessageBox::StandardButton);

    switch (type) {
        case NotificationType::Message:
            messageFunction = QMessageBox::information;
            break;

        case NotificationType::Warning:
            messageFunction = QMessageBox::warning;
            break;

        case NotificationType::Error:
            messageFunction = QMessageBox::critical;
            break;
    }

    messageFunction(nullptr, applicationDisplayName(), message, QMessageBox::Ok, QMessageBox::NoButton);
}
