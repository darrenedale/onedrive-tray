//
// Created by darren on 30/07/22.
//

#include <iostream>
#include <QtCore/QLatin1String>
#include <QtCore/QLocale>
#include <QtCore/QProcess>
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
#include "MessagesWindow.h"

using namespace OneDrive;

namespace
{
    const QString DefaultOneDriveConfigFile = "/onedrive/config";
}

Application::Application(int & argc, char ** argv)
        : QApplication(argc, argv),
          m_state(SyncState::Idle),
          m_oneDrivePath(),
          m_oneDriveArgs(),
          m_window(std::make_unique<MessagesWindow>()),
          m_qtTranslator(),
          m_appTranslator(),
          m_trayIcon(nullptr),
          m_trayIconMenu(std::make_unique<QMenu>()),
          m_statusAction(std::make_unique<QAction>(tr("Not started"))),
          m_freeSpaceAction(std::make_unique<QAction>(tr("Free space: "))),
          m_suspendAction(std::make_unique<QAction>(tr("&Suspend synchronization"))),
          m_restartAction(std::make_unique<QAction>(tr("&Restart synchronization"))),
          m_oneDriveProcess(std::make_unique<QProcess>())
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

    loadSettings();
    m_oneDrivePath = parser.value(onedrivePathOption);
    m_oneDriveArgs = parser.value(onedriveArgsOption);

    if (m_oneDrivePath.isEmpty()) {
        m_oneDrivePath = QLatin1String("/usr/bin/onedrive");
    }

    if (m_oneDriveArgs.isEmpty()) {
        m_oneDriveArgs = QLatin1String("--verbose --monitor");
    }

    connect(m_oneDriveProcess.get(), &QProcess::readyReadStandardOutput, this, &Application::readProcessOutput);
    connect(m_oneDriveProcess.get(), &QProcess::readyReadStandardError, this, &Application::readProcessError);

    createTrayIconMenu();
    m_trayIcon = std::make_unique<QSystemTrayIcon>(QIcon(":/tray-icon-mono"));
    m_trayIcon->setContextMenu(m_trayIconMenu.get());
//    refreshTrayIcon();
    connect(m_trayIcon.get(), &QSystemTrayIcon::activated, this, &Application::trayIconActivated);

    setQuitOnLastWindowClosed(false);
}

Application::~Application() noexcept
{
    m_oneDriveProcess->terminate();
    int giveUp = 5;

    while (QProcess::ProcessState::Running == m_oneDriveProcess->state() && 0 < giveUp) {
        if (m_oneDriveProcess->waitForFinished(1000)) {
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

const QString & Application::oneDrivePath() const
{
    return m_oneDrivePath;
}

const QString & Application::oneDriveArgs() const
{
    return m_oneDriveArgs;
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
    // these are just labels containing status info
    m_freeSpaceAction->setDisabled(true);
    m_statusAction->setDisabled(true);

    m_trayIconMenu->addAction(m_freeSpaceAction.get());
    m_trayIconMenu->addAction(m_statusAction.get());
    m_trayIconMenu->addSeparator();

    auto * action = new QAction(tr("&Recent m_eventsList"), this);
    connect(action, &QAction::triggered, m_window.get(), &QWidget::showNormal);
    m_trayIconMenu->addAction(action);

    action = new QAction(tr("&Open OneDrive folder"), this);
    connect(action, &QAction::triggered, this, &Application::openLocalDirectory);
    m_trayIconMenu->addAction(action);

    connect(m_restartAction.get(), &QAction::triggered, this, &Application::restartProcess);
    m_restartAction->setVisible(false);

    connect(m_suspendAction.get(), &QAction::triggered, this, &Application::suspendProcess);

    auto * iconColorGroup = new QActionGroup(this);

    action = iconColorGroup->addAction(QIcon(":/tray-icon-mono"), tr("Monochrome"));
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

    m_trayIconMenu->addSeparator();

    auto * submenuColor = m_trayIconMenu->addMenu(tr("Icon style"));
    submenuColor->addActions(iconColorGroup->actions());

    m_trayIconMenu->addSeparator();
    m_trayIconMenu->addAction(m_restartAction.get());
    m_trayIconMenu->addAction(m_suspendAction.get());

    m_trayIconMenu->addSeparator();
    action = new QAction(tr("&About OneDrive"), this);
    connect(action, &QAction::triggered, this, &Application::showAboutDialogue);
    m_trayIconMenu->addAction(action);

    action = new QAction(tr("&Quit OneDrive"), this);
    connect(action, &QAction::triggered, this, &Application::quit);
    m_trayIconMenu->addAction(action);
}

void Application::setTrayIconStyle(IconStyle style)
{
    settings.iconStyle = style;
    saveSettings();
    refreshTrayIcon();
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
    static QString path = locateOneDriveConfigFile();
    return path;
}

QString Application::locateOneDriveConfigFile() const
{
    QStringList args = oneDriveArgs().split(QRegularExpression(" +"));
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

    // Open the folder
    auto * openFolderProcess = new QProcess();
    openFolderProcess->start("xdg-open", {path});
    connect(openFolderProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), openFolderProcess, &QProcess::deleteLater);
}

int Application::exec()
{
    m_oneDriveProcess->setProgram(oneDrivePath());
    m_oneDriveProcess->setArguments(oneDriveArgs().split(' ', Qt::SkipEmptyParts));

    m_trayIcon->show();
    refreshTrayIcon();
    m_oneDriveProcess->start();

    return QApplication::exec();
}

Application::ProcessMessage Application::parseProcessOutputLine(const QByteArray & line)
{
    ProcessMessage message;

    if (line.toLower().contains("remaining free space")) {
        message.type = ProcessMessageType::FreeSpace;

        if (auto match = QRegularExpression("([0-9]+)").match(line); match.hasMatch()) {
            message.size = match.captured(1).toLongLong();
        }
    } else if (line.contains("Sync with OneDrive is complete")) {
        message.type = ProcessMessageType::Finished;
    } else if (line.contains("Monitored directory removed")) {
        message.type = ProcessMessageType::LocalRootDirectoryRemoved;
    } else if (line.left(26) == "Creating local directory: ") {
        message.type = ProcessMessageType::CreateLocalDir;
        message.destination = line.right(line.size() - 26);
    } else if (line.left(42) == "Successfully created the remote directory ") {
        message.type = ProcessMessageType::CreateRemoteDir;
        // Remove " on OneDrive" at the end
        message.destination = line.mid(42, line.size() - 54);
    } else if (line.left(7) == "Moving ") {
        message.type = ProcessMessageType::Rename;

        if (const auto match = QRegularExpression("^Moving (.+) to (.+)$").match(line); match.hasMatch()) {
            message.source = match.captured(1);
            message.destination = match.captured(2);
        }
    } else if (auto match = QRegularExpression("(Downloading (?:new |modified)?file|Uploading (?:new |modified )?file|Deleting item)").match(line); match.hasMatch()) {
        const auto type = match.captured(1);

        if (type == "Deleting item") {
            message.type = ProcessMessageType::Delete;

            if (match = QRegularExpression("(?: item from OneDrive:| item) (.+)$").match(line); match.hasMatch()) {
                message.destination = match.captured(1);
            }
        } else if (type.contains("Uploading")) {
            message.type = ProcessMessageType::Upload;

            if (match = QRegularExpression(R"((?:Uploading (?:new |modified )?file) (.+) \.\.\.)").match(line); match.hasMatch()) {
                auto fileName = match.captured(1);

                if (fileName.endsWith(" ...")) {
                    fileName.truncate(fileName.size() - 4);
                }

                message.destination = fileName;
            }
        } else if (line.contains("Downloading")) {
            message.type = ProcessMessageType::Download;

            if (match = QRegularExpression(R"((?:Downloading (?:new |modified )?file) (.+) \.\.\.)").match(line); match.hasMatch()) {
                auto fileName = match.captured(1);

                if (fileName.endsWith(" ...")) {
                    fileName.truncate(fileName.size() - 4);
                }

                message.destination = fileName;
            }
        }
    }

    return message;
}

void Application::readProcessOutput()
{
    static QByteArray buffer;

    buffer += m_oneDriveProcess->readAllStandardOutput();

    for (const QByteArray & line : buffer.split('\n')) {
        const auto message = parseProcessOutputLine(line);

        switch (message.type) {
            case ProcessMessageType::Unknown:
                m_state = SyncState::Idle;
                break;

            case ProcessMessageType::FreeSpace:
                m_state = SyncState::Idle;
                m_statusAction->setText(tr("Sync complete"));
                m_freeSpaceAction->setText(tr("Free space: %1").arg( QLocale::system().formattedDataSize(static_cast<qint64>(message.size), 2, QLocale::DataSizeTraditionalFormat)));
                refreshTrayIcon();
                Q_EMIT freeSpaceUpdated(message.size);
                Q_EMIT syncComplete();
                break;

            case ProcessMessageType::Finished:
                m_state = SyncState::Idle;
                m_statusAction->setText(tr("Sync complete"));
                refreshTrayIcon();
                Q_EMIT syncComplete();
                break;

            case ProcessMessageType::LocalRootDirectoryRemoved:
                m_state = SyncState::Idle;
                m_statusAction->setText(tr("Sync complete"));
                refreshTrayIcon();
                Q_EMIT localRootDirectoryRemoved();
                break;

            case ProcessMessageType::CreateLocalDir:
                m_state = SyncState::Syncing;
                m_statusAction->setText(tr("Local directory %1 created").arg(message.destination));
                Q_EMIT localDirectoryCreated(message.destination);
                break;

            case ProcessMessageType::CreateRemoteDir:
                m_state = SyncState::Syncing;
                m_statusAction->setText(tr("Remote directory %1 created").arg(message.destination));
                Q_EMIT remoteDirectoryCreated(message.destination);
                break;

            case ProcessMessageType::Delete:
                m_state = SyncState::Syncing;
                m_statusAction->setText(tr("File %1 deleted").arg(message.destination));
                Q_EMIT fileDeleted(message.destination);
                break;

            case ProcessMessageType::Rename:
                m_state = SyncState::Syncing;
                m_statusAction->setText(tr("File %1 renamed as %2").arg( message.source, message.destination));
                Q_EMIT fileRenamed(message.source, message.destination);
                break;

            case ProcessMessageType::Upload:
                m_state = SyncState::Syncing;
                m_statusAction->setText(tr("Synchronizing..."));
                Q_EMIT fileUploaded(message.destination);
                break;

            case ProcessMessageType::Download:
                m_state = SyncState::Syncing;
                m_statusAction->setText(tr("Synchronizing..."));
                Q_EMIT fileDownloaded(message.destination);
                break;
        }
    }

    if (const int lastLineEnd = buffer.lastIndexOf('\n'); 0 <= lastLineEnd) {
        buffer = buffer.right(buffer.size() - lastLineEnd);
    } else {
        buffer.clear();
    }
}

void Application::readProcessError()
{
//    QByteArray strdata = m_oneDriveProcess->readAllStandardError();
//    addErrorMessage(QString(strdata));
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

void Application::suspendProcess()
{
    m_oneDriveProcess->terminate(); // Kill m_oneDriveProcess.
    m_oneDriveProcess->waitForFinished(); // Wait for m_oneDriveProcess finish
    m_restartAction->setVisible(true);
    m_suspendAction->setVisible(false);
    refreshTrayIcon();

    m_statusAction->setText(tr("Synchronization suspended"));
    Q_EMIT processStopped();
}

void Application::restartProcess()
{
    m_oneDriveProcess->start(); // Start the m_oneDriveProcess.
    m_suspendAction->setVisible(true);
    m_restartAction->setVisible(false);
    Q_EMIT processStarted();
}

void Application::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Context:
        case QSystemTrayIcon::Unknown:
            break;

        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            if (m_window->isVisible()) {
                showWindow();
            } else {
                hideWindow();
            }
            break;

        case QSystemTrayIcon::MiddleClick:
            qint64 pid = m_oneDriveProcess->processId();

            if (pid > 0) {
                showNotification(tr("OneDrive is running with the PID %1.").arg(pid));
            } else {
                showNotification(tr("OneDrive is not running by some reason. Please restart the program."));
            }
            break;
    }
}

void Application::showWindow()
{
    m_window->setVisible(true);
    m_window->activateWindow();
    m_window->raise();
}

void Application::hideWindow()
{
    m_window->hide();
}
