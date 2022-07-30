
#include <iostream>
#include <stdexcept>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFrame>
#include "IconStyle.h"
#include "Window.h"
#include "Application.h"

using namespace OneDrive;

Window::Window()
{
    // Used to show the window in odd clicks and hide in even.
    auto_hide = true;
    // Used to know if OneDrive is syncing or not
    isSyncing = false;

    // Load the settings of the application
    loadSettings();

    createMessageGroupBox();
    createActions();
    createTrayIcon();

    auto * mainLayout = new QVBoxLayout;
    mainLayout->addWidget(messageGroupBox);
    setLayout(mainLayout);

    setWindowTitle(tr("Recent events"));
    resize(appConfig.size);
    if (!appConfig.pos.isNull()) {
        move(appConfig.pos);
    }

    execute(oneDriveApp->oneDrivePath(), oneDriveApp->oneDriveArgs());
    eventsInfo(tr("OneDrive started"));
}

void Window::execute(QString onedrive_path, QString onedrive_arguments)
{
    process = new QProcess();

    if (onedrive_path.isEmpty()) {
        onedrive_path = QString("onedrive");
    }

    if (onedrive_arguments.isEmpty()) {
        onedrive_arguments = QString("--verbose --monitor");
    }

    qDebug() << "Selected onedrive path: " << onedrive_path;
    qDebug() << "Selected onedrive args: " << onedrive_arguments;

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QStringList arguments = onedrive_arguments.split(" ", QString::SkipEmptyParts);
#else
    QStringList arguments = onedrive_arguments.split(" ", Qt::SkipEmptyParts);
#endif
    process->setProgram(onedrive_path);
    process->setArguments(arguments);

    process->start();

    QObject::connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readStdOutput()));
    QObject::connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readStdError()));
}

void Window::openFolder()
// Open the OneDrive local folder 
{
    // Define the OneDrive config filename
    QString onedriveConfigFileName("");
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QStringList arguments01 = oneDriveApp->oneDriveArgs().split(" ", QString::SkipEmptyParts);
#else
    QStringList arguments01 = oneDriveApp->oneDriveArgs().split(QRegularExpression("[ =]"), Qt::SkipEmptyParts);
#endif
    int index = arguments01.indexOf("--confdir", 0) + 1;
    if (index > 0) {
        onedriveConfigFileName = arguments01[index] + "/config";
        if (onedriveConfigFileName.indexOf("~/") == 0) {
            onedriveConfigFileName.replace(0, 1, QDir::homePath());
        }
    } else {
        onedriveConfigFileName = QDir::homePath() + "/.config/onedrive/config";
    }

    // Extract the onedrive folder in the config file.
    QSettings settings(onedriveConfigFileName, QSettings::IniFormat);
    QString syncDirString = settings.value("sync_dir", "~/OneDrive").toString();
    if (!syncDirString.isEmpty()) {
        // Replace ~/ by home user directory
        if (syncDirString.indexOf("~/") == 0) {
            syncDirString.replace(0, 1, QDir::homePath());
        }
        // Open the folder
        auto * openFolderProcess = new QProcess(this);
        openFolderProcess->start("xdg-open", QStringList() << syncDirString);
    }
}

void Window::suspend()
// Stop OneDrive
{
    process->terminate(); // Kill process.
    process->waitForFinished(); // Wait for process finish
    restartAction->setVisible(true);
    suspendAction->setVisible(false);
    isSyncing = false;
    refreshTrayIcon(true, false);

    statusAction->setText(tr("Synchronization suspended"));
    eventsInfo(tr("Synchronization suspended"));
}

void Window::restart()
// Restart OneDrive
{
    process->start(); // Start the process.
    suspendAction->setVisible(true);
    restartAction->setVisible(false);

    eventsInfo(tr("Synchronization restarted"));
}

void Window::setTrayIconStyle(
        IconStyle & style)
// Slot function to define the icon of the tray icon
{
    appConfig.iconStyle = style;
    refreshTrayIcon(true, false);
}

void Window::refreshTrayIcon(bool forceChange, bool sync)
{
    // do not change the tray icon if is currently syncing and continue to sync
    if (forceChange || !(isSyncing && sync)) {
        if (sync || (isSyncing && forceChange)) {
            trayIcon->setIcon(QIcon(":/tray-icon-sync"));
        } else {
            switch (appConfig.iconStyle) {
                case IconStyle::colourful:
                    trayIcon->setIcon(QIcon(":/tray-icon-colour"));
                    break;

                case IconStyle::monochrome:
                    trayIcon->setIcon(QIcon(":/tray-icon-mono"));
                    break;

                default:
                    throw std::logic_error("Unhandled icon style in Window::refreshTrayIcon()");
            }
        }
    }
}

void Window::readStdOutput()
// Slot fonction used to read the informations of the standard output and to process the informations
{
    bool syncCompleted(false);

    if (!isSyncing) {
        statusAction->setText(tr("Synchronizing..."));
        refreshTrayIcon(false, true);
        isSyncing = true;
    }

    QByteArray strdata = process->readAllStandardOutput();
    *stdOutputString = *stdOutputString + QString(strdata);

    if (stdOutputString->right(1) == "\n") {
        QRegularExpression re;
        QRegularExpressionMatch match;
        QString operation("");
        QString fileName("");

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
        QStringList listLine = stdOutputString->split("\n", QString::SkipEmptyParts);
#else
        QStringList listLine = stdOutputString->split("\n", Qt::SkipEmptyParts);
#endif
        for (int i = 0; i < listLine.size(); i++) {
            // For debugging, you can de-comment the line under to see all the messages in the recent events window
            // eventsInfo(listLine[i]);

            // Get the free space on OneDrive and update the menu
            if (listLine[i].toLower().contains("remaining free space")) {
                re.setPattern("([0-9]+)");
                match = re.match(listLine[i]);
                if (match.hasMatch()) {
                    qint64 freeSpace = match.captured(1).toLongLong();
                    freeSpaceAction->setText(tr("Free space: ") + QLocale().formattedDataSize(freeSpace, 2, QLocale::DataSizeTraditionalFormat));
                }
                syncCompleted = true;
            }
                // Sync is completed
            else if (listLine[i].contains("Sync with OneDrive is complete") ||
                     listLine[i].contains("Monitored directory removed")) {
                syncCompleted = true;
                // Create local directory
            } else if (listLine[i].left(26) == "Creating local directory: ") {
                operation = tr("Creating local directory");
                fileName = listLine[i];
                fileName.remove(0, 26);
                syncCompleted = true;
            }
                // Create directory on OneDrive
            else if (listLine[i].left(42) == "Successfully created the remote directory ") {
                operation = tr("Create directory");
                fileName = listLine[i];
                fileName.remove(0, 42);
                fileName.chop(12); // Remove " on OneDrive" at the end
                syncCompleted = true;
            }
                // Create directory on OneDrive
            else if (listLine[i].left(7) == "Moving ") {
                operation = tr("Rename");
                re.setPattern("^Moving (.+) to (.+)$");
                match = re.match(listLine[i]);
                if (match.hasMatch()) {
                    fileName = match.captured(1) + " --> " + match.captured(2);
                }
                syncCompleted = true;
            }
                // Uploading, downloading or deleting files
            else if (listLine[i].contains(QRegularExpression(
                    "Downloading file|Downloading new file|Downloading modified file|Uploading file|Uploading new file|Uploading modified file|Deleting item"))) {
                if (listLine[i].contains("Deleting")) {
                    operation = tr("Deleting");
                    re.setPattern("( item from OneDrive:| item) (.+)$");
                    match = re.match(listLine[i]);
                    if (match.hasMatch()) {
                        fileName = match.captured(2);
                    }
                    syncCompleted = true;
                } else if (listLine[i].contains("Uploading")) {
                    operation = tr("Uploading");
                    re.setPattern(R"((Uploading file|Uploading new file|Uploading modified file) (.+) \.\.\.)");
                    match = re.match(listLine[i]);
                    if (match.hasMatch()) {
                        fileName = match.captured(2);
                        if (fileName.endsWith(" ...")) {
                            fileName.truncate(fileName.size() - 4);
                        }
                    }
                } else if (listLine[i].contains("Downloading")) {
                    operation = tr("Downloading");
                    re.setPattern(R"((Downloading file|Downloading new file|Downloading modified file) (.+) \.\.\.)");
                    match = re.match(listLine[i]);
                    if (match.hasMatch()) {
                        fileName = match.captured(2);
                        if (fileName.endsWith(" ...")) {
                            fileName.truncate(fileName.size() - 4);
                        }
                    }
                }
            }

            if (!operation.isEmpty()) {
                eventsOperation(operation, fileName);
            }

            if (syncCompleted) {
                statusAction->setText(tr("Sync complete"));
                refreshTrayIcon(false, false);
                isSyncing = false;
            }

            operation = "";
            fileName = "";
        }
        *stdOutputString = "";
    }
}

void Window::readStdError()
// Slot fonction used to read the informations of the standard error
{
    QByteArray strdata = process->readAllStandardError();
    eventsError(QString(strdata));
}

void Window::closeEvent(QCloseEvent * event)
// Event launched when the window is closing
{
#ifdef Q_OS_OSX
    if (!event->spontaneous() || !isVisible()) {
        return;
    },
#endif
    if (trayIcon->isVisible()) {
        appConfig.size = size();
        appConfig.pos = pos();
        hide();
        event->ignore();
    }
}

void Window::quit()
{
    process->terminate();
    saveSettings();
    qApp->quit();
}

void Window::about()
// Function responsible to show the About dialog box
{
}

void Window::iconActivated(QSystemTrayIcon::ActivationReason reason)
// Actions realized when you click on the tray icon (double click, right click, left click...)
{
    switch (reason) {
        case QSystemTrayIcon::Context:
        case QSystemTrayIcon::Unknown:
            break;
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            // Put here what should call when click with left button
            if (auto_hide) {
                this->showNormal(); // Show the information window.
            } else {
                this->hide();
            } // Show the information window.
            auto_hide = !auto_hide;
            break;
        case QSystemTrayIcon::MiddleClick: // Put here what should call when click with mid button
            // Get process identifier (PID).
            qint64 pid = process->processId();

            // Create the string to store the informations.
            QString text;

            // If pid is greater than 0 the process is running
            // and when show a info about this.
            if (pid > 0) {
                text.append(tr("OneDrive is running with the PID "));
                text.append(QString::number(pid));
                text.append(".\n");
            }
                // If not the process is stopped and when notify the user.
            else
                text.append(tr("OneDrive is not running by some reason. Please restart the program."));

            oneDriveApp->showNotification(text);
            break;
    }
}

void Window::eventsInfo(QString info)
// Add an info in the recent events
{
    events->appendHtml("<p><span style=\"color:gray;\">" + QDateTime::currentDateTime().toString(QLocale::system().dateFormat(QLocale::ShortFormat)) +
                       " </span><span style=\"color:black;\"> " + info + "</span></p>");
}

void Window::eventsError(QString error)
// Add an error in the recent events
{
    events->appendHtml("<p><span style=\"color:gray;\">" + QDateTime::currentDateTime().toString(QLocale::system().dateFormat(QLocale::ShortFormat)) +
                       " </span><span style=\"color:red;\"> " + error + "</span></p>");
}

void Window::eventsOperation(QString operation, QString fileName)
// Add an operation (downloading, uploading, removing...) in the recent events
{
    events->appendHtml("<p><span style=\"color:gray;\">" + QDateTime::currentDateTime().toString(QLocale::system().dateFormat(QLocale::ShortFormat)) +
                       "</span><span style=\"color:blue;\"> " + operation + ", <b>" + fileName + "</b> </span></p>");
}

void Window::createMessageGroupBox()
// Create the recent events message box
{
    // ********** Recent events Output Area **********
    events = new QPlainTextEdit;
    events->setReadOnly(true);
    events->setMaximumBlockCount(5000);

    QGridLayout * messageLayout = new QGridLayout;
    messageLayout->addWidget(events, 2, 1, 1, 4);
    messageLayout->setColumnStretch(3, 0);
    messageLayout->setRowStretch(4, 0);

    messageGroupBox = new QGroupBox(tr("Recent events"));
    messageGroupBox->setLayout(messageLayout);

    // ******* Create the standard output string
    stdOutputString = new QString("");

    // ********** Create the baloon warning in the tray ********** //
    //?????? TO DO
    showIconCheckBox = new QCheckBox(tr("Show icon"));
    showIconCheckBox->setChecked(true);
}

void Window::createActions()
{
    freeSpaceAction = new QAction(tr("Free space: "), this);
    freeSpaceAction->setDisabled(true);

    statusAction = new QAction(tr("Not started"), this);
    statusAction->setDisabled(true);

    consoleAction = new QAction(tr("&Recent events"), this);
    connect(consoleAction, &QAction::triggered, this, &QWidget::showNormal);

    openfolderAction = new QAction(tr("&Open OneDrive folder"), this);
    connect(openfolderAction, &QAction::triggered, this, &Window::openFolder);

    restartAction = new QAction(tr("&Restart the synchronization"), this);
    connect(restartAction, &QAction::triggered, this, &Window::restart);
    restartAction->setVisible(false);

    suspendAction = new QAction(tr("&Suspend the synchronization"), this);
    connect(suspendAction, &QAction::triggered, this, &Window::suspend);

    iconColorGroup = new QActionGroup(this);

    auto * action = iconColorGroup->addAction(QIcon(":/tray-icon-mono"), tr("Monochrome"));
    action->setCheckable(true);
    action->setChecked(IconStyle::colourful == appConfig.iconStyle);

    connect(action, &QAction::triggered,[this] {
        setTrayIconStyle(IconStyle::monochrome);
    });

    action = iconColorGroup->addAction(QIcon(":/tray-icon-colour"), tr("Colourful"));
    action->setCheckable(true);
    action->setChecked(IconStyle::colourful == appConfig.iconStyle);

    connect(action, &QAction::triggered,[this] {
        setTrayIconStyle(IconStyle::colourful);
    });

    quitAction = new QAction(tr("&Quit OneDrive"), this);
    connect(quitAction, &QAction::triggered, this, &Window::quit);

    aboutAction = new QAction(tr("&About OneDrive"), this);
    connect(aboutAction, &QAction::triggered, oneDriveApp, &Application::showAboutDialogue);
}

void Window::createTrayIcon()
{
    trayIconMenu = new QMenu(this);

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

void Window::loadSettings()
{
    QSettings settings;

    switch (settings.value("Tray/IconStyle", 0).value<int>()) {
        default:
            std::cerr << "unexpected icon style " << settings.value("Tray/IconStyle", 0).value<int>() << " in settings file - defaulting to 'colourful'\n";
            [[fallthrough]];
        case static_cast<int>(IconStyle::colourful):
            appConfig.iconStyle = IconStyle::colourful;
            break;

        case static_cast<int>(IconStyle::monochrome):
            appConfig.iconStyle = IconStyle::monochrome;
            break;
    }

    settings.beginGroup("RecentEventWindow");
    appConfig.size = settings.value("Size", QSize(400, 300)).toSize();
    appConfig.pos = settings.value("Position", QPoint(0, 0)).toPoint();
    settings.endGroup();
}

void Window::saveSettings()
{
    QSettings settings;
    settings.setValue("Tray/IconStyle", static_cast<int>(appConfig.iconStyle));
    settings.beginGroup("RecentEventWindow");
    settings.setValue("Size", appConfig.size);
    settings.setValue("Position", appConfig.pos);
    settings.endGroup();
}
