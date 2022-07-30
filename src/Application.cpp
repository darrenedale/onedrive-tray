//
// Created by darren on 30/07/22.
//

#include <iostream>
#include <QtCore/QLatin1String>
#include <QtCore/QLocale>
#include <QtCore/QCommandLineParser>
#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMenu>
#include "Application.h"
#include "Window.h"

using namespace OneDrive;

namespace
{
    static const QString DefaultOneDriveConfigFile = "/onedrive/config";
}

Application::Application(int & argc, char ** argv)
        : QApplication(argc, argv),
          m_state(SyncState::Idle),
          m_oneDrivePath(),
          m_oneDriveArgs(),
          m_window(),
          m_qtTranslator(),
          m_appTranslator(),
          m_trayIcon(std::make_unique<QSystemTrayIcon>())
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
    m_window = std::make_unique<Window>();
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

void Application::showNotification(const QString & message, int timeout, OneDrive::Application::NotificationType type)
{
    QMessageBox::StandardButton (* messageFunction)(QWidget *, const QString &, const QString &, QMessageBox::StandardButtons, QMessageBox::StandardButton);

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

void Application::showAboutDialogue() const
{
    QMessageBox::about(nullptr, tr("About"),
                       "<b>" + applicationName() + " " + applicationVersion() + "</b><br><br>" +
                       tr("Run and control OneDrive from the system tray.<br>"
                          "This is a simple program to create a system tray icon and display program status for onedrive client developed by abraunegg.<br><br>"
                          "Click with the left mouse button or double-click (depends on Linux distro) and the program shows the synchronization progress. "
                          "Click with the right mouse button and a menu with the available options is shown. "
                          "Click with the mid mouse button and the program shows the PID of the onedrive client.<br><br>"
                          "The program was written in C++ using lib Qt 5.13.0.<br><br>"
                          "To use the program you must first compile and install the onedrive client available at https://github.com/abraunegg/onedrive.<br><br>"
                          "So many thanks to<ul>"
                          "<li>abraunegg (https://github.com/abraunegg/onedrive)</li>"
                          "<li>Daniel Borges Oliveira who developed the first version of this program (https://github.com/DanielBorgesOliveira/onedrive_tray)</li></ul><br>"
                          "Feel free to clone and improve this program (https://github.com/bforest76/onedrive_tray).<br>"));
}

void Application::createTrayIconMenu()
{
    auto * freeSpaceAction = new QAction(tr("Free space: "), this);
    freeSpaceAction->setDisabled(true);

    auto * statusAction = new QAction(tr("Not started"), this);
    statusAction->setDisabled(true);

    auto * consoleAction = new QAction(tr("&Recent events"), this);
    connect(consoleAction, &QAction::triggered, this, &QWidget::showNormal);

    auto * openfolderAction = new QAction(tr("&Open OneDrive folder"), this);
    connect(openfolderAction, &QAction::triggered, this, &Window::openFolder);

    auto * restartAction = new QAction(tr("&Restart the synchronization"), this);
    connect(restartAction, &QAction::triggered, this, &Window::restart);
    auto * restartAction->setVisible(false);

    auto * suspendAction = new QAction(tr("&Suspend the synchronization"), this);
    connect(suspendAction, &QAction::triggered, this, &Window::suspend);

    auto * iconColorGroup = new QActionGroup(this);

    auto * action = iconColorGroup->addAction(QIcon(":/tray-icon-mono"), tr("Monochrome"));
    action->setCheckable(true);
    action->setChecked(IconStyle::colourful == settings.iconStyle);

    connect(action, &QAction::triggered,[this] {
        setTrayIconStyle(IconStyle::monochrome);
    });

    action = iconColorGroup->addAction(QIcon(":/tray-icon-colour"), tr("Colourful"));
    action->setCheckable(true);
    action->setChecked(IconStyle::colourful == settings.iconStyle);

    connect(action, &QAction::triggered,[this] {
        setTrayIconStyle(IconStyle::colourful);
    });

    quitAction = new QAction(tr("&Quit OneDrive"), this);
    connect(quitAction, &QAction::triggered, this, &Window::quit);

    aboutAction = new QAction(tr("&About OneDrive"), this);
    connect(aboutAction, &QAction::triggered, oneDriveApp, &Application::showAboutDialogue);

    auto * trayIconMenu = new QMenu(m_trayIcon.get());

    trayIconMenu->addAction(freeSpaceAction);
    trayIconMenu->addAction(statusAction);
    trayIconMenu->addSeparator();

    trayIconMenu->addAction(consoleAction);
    trayIconMenu->addAction(openfolderAction);
    trayIconMenu->addAction(restartAction);
    trayIconMenu->addAction(suspendAction);
    trayIconMenu->addSeparator();

    QMenu* submenuColor = trayIconMenu->addMenu(tr("Icon style"));
    submenuColor->addActions(iconColorGroup->actions());

    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
    trayIconMenu->addAction(aboutAction);

    trayIcon = new QSystemTrayIcon(this);
    refreshTrayIcon(true, false);

    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated, this, &Window::iconActivated);
}

void Application::setTrayIconStyle(IconStyle style)
{
    settings.iconStyle = style;
    saveSettings();
    refreshTrayIcon(true, false);
}

void Application::saveSettings() const
{
    QSettings settingsStore;
    settingsStore.beginGroup(QLatin1String("Application"));
    settingsStore.setValue("iconStyle", static_cast<int>(settings.iconStyle));
    settingsStore.endGroup();
}

void Application::loadSettings()
{
    QSettings settingsStore;
    settingsStore.beginGroup(QLatin1String("Application"));

    switch (settingsStore.value("iconStyle", 0).value<int>()) {
        default:
            std::cerr << "unexpected icon style " << settingsStore.value("iconStyle", 0).value<int>() << " in settings file - defaulting to 'colourful'\n";
            [[fallthrough]];
        case static_cast<int>(IconStyle::colourful):
            settings.iconStyle = IconStyle::colourful;
            break;

        case static_cast<int>(IconStyle::monochrome):
            settings.iconStyle = IconStyle::monochrome;
            break;
    }

    settingsStore.endGroup();
}

IconStyle Application::iconStyle() const
{
    return settings.iconStyle;
}

void Application::refreshTrayIcon()
{
    // do not change the tray icon if is currently syncing and continue to sync
    QString iconName = ":/tray-icon";

    switch (state()) {
        case SyncState::Idle:
            // no suffix when idle
            break;

        case SyncState::Syncing:
            iconName += "-sync";
            break;

        default:
            throw std::logic_error("Unhandled sync state in Application::refreshTrayIcon()");
    }

    switch (settings.iconStyle) {
        case IconStyle::colourful:
            iconName += "-colour";
            break;

        case IconStyle::monochrome:
            iconName += "-mono";
            break;

        default:
            throw std::logic_error("Unhandled icon style in Application::refreshTrayIcon()");
    }

    m_trayIcon->setIcon(QIcon(iconName));
}

const QString & Application::oneDriveConfigFile() const
{
    static QString path;

    if (path.isNull()) {
        path =
    }
}

QString Application::determineOneDriveConfigFile()
{
    QStringList args = oneDriveArgs().split(QRegularExpression(" +"));
    auto it = std::find(args.cbegin(), args.cend(), QStringLiteral("--confdir"));
    QString path;

    if (it == args.cend()) {
        path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + DefaultOneDriveConfigFile;
    } else {
        ++it;
        path = *it;

        if (path.startsWith(QLatin1String("~/"))) {
            path.replace(0, 1, QDir::homePath());
        } else if (path.startsWith(QLatin1String("$HOME/"))) {
            path.replace(0, 5, QDir::homePath());
        } else if (path.startsWith(QLatin1String("${HOME}/"))) {
            path.replace(0, 7, QDir::homePath());
        }

        // TODO read the config file and locate the local directory
    }
}

void Application::openLocalDirectory()
{

}
