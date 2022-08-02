/**
 * Application.cpp
 *
 * Implementation of Application class.
 */

#include <iostream>
#include <QtCore/QLatin1String>
#include <QtCore/QLocale>
#include <QtCore/QCommandLineParser>
#include <QtCore/QSettings>
#include <QtCore/QLibraryInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMenu>
#include "Application.h"
#include "Process.h"

using namespace OneDrive;

namespace
{
    const QString DefaultOneDriveConfigFile = QStringLiteral("/onedrive/config");
    const QString DefaultOneDrivePath = QStringLiteral("/usr/bin/onedrive");
    const QStringList FixedOneDriveArguments = {"--verbose", "--monitor"};
    const QString DefaultIcon = QStringLiteral(":/tray-icon-mono");
}


Application::Application(int & argc, char ** argv)
        : QApplication(argc, argv),
          m_oneDrivePath(),
          m_oneDriveArguments(FixedOneDriveArguments),
          m_oneDriveProcess(),
          m_window(m_oneDriveProcess),
          m_trayIcon(QIcon(DefaultIcon)),
          m_trayIconMenu(),
          m_statusAction(tr("Not started")),
          m_freeSpaceAction(tr("Free space: ")),
          m_suspendAction(tr("&Suspend synchronization")),
          m_restartAction(tr("&Restart synchronization")),
          m_settings(),
          m_qtTranslator(),
          m_appTranslator()
{
    installTranslators();

    setAttribute(Qt::AA_UseHighDpiPixmaps);
    setOrganizationDomain(QLatin1String("dev.equit"));
    setOrganizationName(QLatin1String("Équit"));
    setApplicationDisplayName(tr("OneDrive synchronisation app"));
    setApplicationVersion(QLatin1String("3.0"));
    QApplication::setWindowIcon(QIcon(":window-icon"));

    // read command-line args
    QCommandLineParser parser;
    parser.setApplicationDescription(tr("Synchronise your OneDrive from the system tray."));
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption(QCommandLineOption(
            {"p" , "onedrive-path"},
            "Path to the OneDrive program.",
            "path",
            DefaultOneDrivePath
    ));

    parser.addOption(QCommandLineOption(
            {"a", "onedrive-args"},
            "Custom arguments to pass to the onedrive client. The arguments --verbose and --monitor are always passed",
            "args"
    ));

    parser.addOption(QCommandLineOption(
            {"s", "silent-fail"},
            tr("Silently quit if no system tray is available rather than showing a notification first.")
    ));

    parser.addOption(QCommandLineOption(
            {"d", "debug"},
            tr("Output more information to stdout while running.")
    ));

    parser.process(*this);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        if (!parser.isSet(QLatin1String("silent-fail"))) {
            showNotification(tr("No system tray is available."));
        }

        throw RuntimeException("No system tray.");
    }

#if defined(NDEBUG)
    m_debug = parser.isSet(QLatin1String("debug"));
#else
    m_debug = true;
#endif

    m_oneDrivePath = parser.value(QLatin1String("p"));

    if (m_oneDrivePath.isEmpty()) {
        m_oneDrivePath = DefaultOneDrivePath;
    }

    if (const auto args = parser.value(QLatin1String("a")); !args.isEmpty()) {
        m_oneDriveArguments.append(args.split(QRegularExpression(QLatin1String(" +")), Qt::SplitBehaviorFlags::SkipEmptyParts));
    }

    loadSettings();

    setupTrayIconMenu();
    connect(&m_trayIcon, &QSystemTrayIcon::activated, this, &Application::trayIconActivated);
    setQuitOnLastWindowClosed(false);

    connectProcess();
}


Application::~Application() noexcept
{
    m_oneDriveProcess.disconnect(this);
    m_oneDriveProcess.terminate();
    int giveUp = 5;

    while (QProcess::ProcessState::Running == m_oneDriveProcess.state() && 0 < giveUp) {
        if (m_oneDriveProcess.waitForFinished(1000)) {
            break;
        }

        std::cerr << "waited 1s for onedrive process to finish.\n";
        --giveUp;
    }

    if (0 == giveUp) {
        std::cerr << "onedrive process did not terminate cleanly.\n";
    }
}


void Application::installTranslators()
{
    if (m_qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        installTranslator(&m_qtTranslator);
    }

    // Loads path + filename + prefix + ui language name + suffix (".qm" if the suffix is not specified)
    if (m_appTranslator.load(QLocale(), applicationName(), "_", applicationDirPath())) {
        installTranslator(&m_appTranslator);
    } else {
        std::cerr <<
            "Translation not found for " <<
            qPrintable(QLocale::languageToString(QLocale::system().language())) <<
            " language (" <<
            qPrintable(std::reduce(
                    QLocale::system().uiLanguages().cbegin(),
                    QLocale::system().uiLanguages().cend(),
                    QString(),
                    [](QString init, const QString & isoLanguage) -> QString {
                        if (!init.isNull()) {
                            init += ", ";
                        }

                        init += isoLanguage;
                        return init;
                    }
            ))
            << ")\n";
    }
}


void Application::showNotification(const QString & message, int timeout, OneDrive::Application::NotificationType type) const
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


void Application::showAboutDialogue()
{
    QMessageBox::about(
        nullptr,
        tr("About"),
        "<b>" + applicationName() + " " + applicationVersion() + "</b><br><br>" +
        tr(
            "Run and control OneDrive from the system tray.<br>"
            "This is a simple program to create a system tray icon and display program status for onedrive client developed by abraunegg.<br><br>"
            "Click with the left mouse button or double-click (depends on Linux distro) and the program shows the synchronization progress. "
            "Click with the right mouse button and a menu with the available options is shown. "
            "Click with the mid mouse button and the program shows the PID of the onedrive client.<br><br>"
            "The program was written in C++ using lib Qt 5.13.0.<br><br>"
            "To use the program you must first compile and install the onedrive client available at https://github.com/abraunegg/onedrive.<br><br>"
            "So many thanks to<ul>"
            "<li>abraunegg (https://github.com/abraunegg/onedrive)</li>"
            "<li>Daniel Borges Oliveira who developed the first version of this program (https://github.com/DanielBorgesOliveira/onedrive_tray)</li></ul><br>"
            "Feel free to clone and improve this program (https://github.com/bforest76/onedrive_tray).<br>"
        )
    );
}


void Application::setupTrayIconMenu()
{
    // these are just labels, they're not really actions
    m_freeSpaceAction.setDisabled(true);
    m_statusAction.setDisabled(true);

    m_trayIconMenu.addAction(&m_freeSpaceAction);
    m_trayIconMenu.addAction(&m_statusAction);

    m_trayIconMenu.addSeparator();

    auto * action = new QAction(tr("&Recent events"), this);
    connect(action, &QAction::triggered, &m_window, &QWidget::showNormal);
    m_trayIconMenu.addAction(action);

    action = new QAction(tr("&Open OneDrive folder"), this);
    connect(action, &QAction::triggered, this, &Application::openLocalDirectory);
    m_trayIconMenu.addAction(action);

    connect(&m_restartAction, &QAction::triggered, &m_oneDriveProcess, [this] () {
        assert(!m_oneDriveProcess.isRunning());
        oneDriveProcess().start();
    });

    m_restartAction.setVisible(false);

    connect(&m_suspendAction, &QAction::triggered, this, [this] () {
        if (!oneDriveProcess().isRunning()) {
            return;
        }

        oneDriveProcess().terminate();
    });

    auto * iconColorGroup = new QActionGroup(this);

    action = iconColorGroup->addAction(QIcon(":/tray-icon-mono"), tr("Monochrome"));
    action->setCheckable(true);
    action->setChecked(IconStyle::colourful == m_settings.iconStyle);

    connect(action, &QAction::triggered,[this] {
        setTrayIconStyle(IconStyle::monochrome);
    });

    action = iconColorGroup->addAction(QIcon(":/tray-icon-colour"), tr("Colourful"));
    action->setCheckable(true);
    action->setChecked(IconStyle::colourful == m_settings.iconStyle);

    connect(action, &QAction::triggered,[this] {
        setTrayIconStyle(IconStyle::colourful);
    });

    m_trayIconMenu.addSeparator();

    auto * colourMenu = m_trayIconMenu.addMenu(tr("Icon style"));
    colourMenu->addActions(iconColorGroup->actions());

    m_trayIconMenu.addSeparator();

    m_trayIconMenu.addAction(&m_restartAction);
    m_trayIconMenu.addAction(&m_suspendAction);

    m_trayIconMenu.addSeparator();

    action = new QAction(tr("&About OneDrive"), this);
    connect(action, &QAction::triggered, this, &Application::showAboutDialogue);
    m_trayIconMenu.addAction(action);

    action = new QAction(tr("&Quit OneDrive"), this);
    connect(action, &QAction::triggered, this, &Application::quit);
    m_trayIconMenu.addAction(action);
    m_trayIcon.setContextMenu(&m_trayIconMenu);
}


void Application::setTrayIconStyle(IconStyle style)
{
    m_settings.iconStyle = style;
    saveSettings();
    refreshTrayIcon();
}


void Application::saveSettings() const
{
    QSettings settingsStore;
    settingsStore.beginGroup(QLatin1String("Application"));
    settingsStore.setValue("iconStyle", static_cast<int>(m_settings.iconStyle));
    settingsStore.endGroup();
}


void Application::loadSettings()
{
    QSettings settingsStore;
    settingsStore.beginGroup(QLatin1String("Application"));

    switch (settingsStore.value("iconStyle", 0).value<int>()) {
        default:
            std::cerr << "unexpected icon style " << settingsStore.value("iconStyle", 0).value<int>() << " in m_settings file - defaulting to 'colourful'\n";
            [[fallthrough]];
        case static_cast<int>(IconStyle::colourful):
            m_settings.iconStyle = IconStyle::colourful;
            break;

        case static_cast<int>(IconStyle::monochrome):
            m_settings.iconStyle = IconStyle::monochrome;
            break;
    }

    settingsStore.endGroup();
}


void Application::refreshTrayIcon()
{
    // do not change the tray icon if is currently syncing and continue to sync
    QString iconName = ":/tray-icon";

    switch (m_oneDriveProcess.synchronisationState()) {
        case Process::SynchronisationState::Idle:
            // no suffix when idle
            break;

        case Process::SynchronisationState::Syncing:
            iconName += "-sync";
            break;

        default:
            throw std::logic_error("Unhandled sync state in Application::refreshTrayIcon()");
    }

    switch (m_settings.iconStyle) {
        case IconStyle::colourful:
            iconName += "-colour";
            break;

        case IconStyle::monochrome:
            iconName += "-mono";
            break;

        default:
            throw std::logic_error("Unhandled icon style in Application::refreshTrayIcon()");
    }

    m_trayIcon.setIcon(QIcon(iconName));
}

const QString & Application::oneDriveConfigFile() const
{
    static QString path = locateOneDriveConfigFile();
    return path;
}


QString Application::locateOneDriveConfigFile() const
{
    QStringList args = oneDriveArgs();
    auto it = std::find(args.cbegin(), args.cend(), QStringLiteral("--confdir"));
    QString path;

    if (it == args.cend()) {
        path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + DefaultOneDriveConfigFile;
    } else {
        ++it;
        path = expandHomeShortcut(*it);
    }

    QSettings oneDriveConfig(path, QSettings::IniFormat);
    return expandHomeShortcut(oneDriveConfig.value("sync_dir", QDir::homePath() + "/OneDrive").toString());
}


void Application::openLocalDirectory() const
{
    static QString path = oneDriveConfigFile();

    if (path.isEmpty()) {
        showNotification(tr("The path for the local OneDrive directory is not defined."));
        return;
    }

    auto * openFolderProcess = new QProcess();
    openFolderProcess->start("xdg-open", {path});
    connect(openFolderProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), openFolderProcess, &QProcess::deleteLater);
}


int Application::exec()
{
    m_trayIcon.show();
    refreshTrayIcon();

    m_oneDriveProcess.setProgram(oneDrivePath());
    m_oneDriveProcess.setArguments(oneDriveArgs());
    m_oneDriveProcess.start();

    return QApplication::exec();
}


QString Application::expandHomeShortcut(const QString & path)
{
    if (path.startsWith(QLatin1String("~/"))) {
        return QString(path).replace(0, 1, QDir::homePath());
    } else if (path.startsWith(QLatin1String("$HOME/"))) {
        QString(path).replace(0, 5, QDir::homePath());
    } else if (path.startsWith(QLatin1String("${HOME}/"))) {
        QString(path).replace(0, 7, QDir::homePath());
    }

    return path;
}


void Application::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Context:
        case QSystemTrayIcon::Unknown:
            break;

        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            if (m_window.isVisible()) {
                showWindow();
            } else {
                hideWindow();
            }
            break;

        case QSystemTrayIcon::MiddleClick:
            if (oneDriveProcess().isRunning()) {
                showNotification(tr("OneDrive is running with the PID %1.").arg(m_oneDriveProcess.processId()));
            } else {
                showNotification(tr("OneDrive is not running. Please restart the program."));
            }
            break;
    }
}


void Application::showWindow()
{
    m_window.setVisible(true);
    m_window.activateWindow();
    m_window.raise();
}


void Application::hideWindow()
{
    m_window.hide();
}


void Application::connectProcess()
{
    connect(&m_oneDriveProcess, &Process::started, this, &Application::onProcessStarted);
    connect(&m_oneDriveProcess, &Process::stopped, this, &Application::onProcessStopped);
    connect(&m_oneDriveProcess, &Process::freeSpaceUpdated, this, &Application::onFreeSpaceUpdated);
    connect(&m_oneDriveProcess, &Process::synchronisationComplete, this, &Application::onSynchronisationComplete);
    connect(&m_oneDriveProcess, &Process::localRootDirectoryRemoved, this, &Application::onLocalRootDirectoryRemoved);
    connect(&m_oneDriveProcess, &Process::localDirectoryCreated, this, &Application::onLocalDirectoryCreated);
    connect(&m_oneDriveProcess, &Process::remoteDirectoryCreated, this, &Application::onRemoteDirectoryCreated);
    connect(&m_oneDriveProcess, &Process::fileRenamed, this, &Application::onFileRenamed);
    connect(&m_oneDriveProcess, &Process::fileDeleted, this, &Application::onFileDeleted);
    connect(&m_oneDriveProcess, &Process::fileUploaded, this, &Application::onFileUploaded);
    connect(&m_oneDriveProcess, &Process::fileDownloaded, this, &Application::onFileDownloaded);
}


void Application::onProcessStarted()
{
    m_restartAction.setVisible(true);
    m_suspendAction.setVisible(false);
    m_statusAction.setText(tr("Synchronization suspended"));
    refreshTrayIcon();
}


void Application::onProcessStopped()
{
    m_suspendAction.setVisible(true);
    m_restartAction.setVisible(false);
    m_statusAction.setText(tr("Idle"));
    refreshTrayIcon();
}


void Application::onFreeSpaceUpdated(quint64 space)
{
    m_freeSpaceAction.setText(tr("Free space: %1").arg( QLocale::system().formattedDataSize(static_cast<qint64>(space), 2, QLocale::DataSizeTraditionalFormat)));
}


void Application::onSynchronisationComplete()
{
    m_statusAction.setText(tr("Sync complete"));
    refreshTrayIcon();
}


void Application::onLocalRootDirectoryRemoved()
{
    m_statusAction.setText(tr("Sync complete"));
    refreshTrayIcon();
}


void Application::onLocalDirectoryCreated(const QString &directoryName)
{
    m_statusAction.setText(tr("Local directory %1 created").arg(directoryName));
}


void Application::onRemoteDirectoryCreated(const QString &directoryName)
{
    m_statusAction.setText(tr("Remote directory %1 created").arg(directoryName));
}


void Application::onFileDeleted(const QString &fileName)
{
    m_statusAction.setText(tr("File %1 deleted").arg(fileName));
}


void Application::onFileRenamed(const QString &from, const QString &to)
{
    m_statusAction.setText(tr("File %1 renamed as %2").arg(from, to));
}


void Application::onFileUploaded(const QString &fileName)
{
    m_statusAction.setText(tr("Uploading %1 ...").arg(fileName));
}


void Application::onFileDownloaded(const QString &fileName)
{
    m_statusAction.setText(tr("Downloading %1 ...").arg(fileName));
}
