
#include <stdexcept>
#include <QtCore/QDateTime>
#include <QtCore/QSettings>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QComboBox>
#include <QtGui/QCloseEvent>
#include <QtCore/QDebug>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include "Window.h"
#include "Application.h"

using namespace OneDrive;

Window::Window()
: QDialog()
{
    loadSettings();
    createMessageGroupBox();

    auto * mainLayout = new QVBoxLayout;
    mainLayout->addWidget(messageGroupBox);
    setLayout(mainLayout);

    setWindowTitle(tr("Recent events"));
    resize(m_settings.size);

    if (!m_settings.pos.isNull()) {
        move(m_settings.pos);
    }

    connectProcess();
    eventsInfo(tr("OneDrive started"));
}

void Window::closeEvent(QCloseEvent * event)
{
#ifdef Q_OS_OSX
    if (!event->spontaneous() || !isVisible()) {
        return;
    },
#endif
    m_settings.size = size();
    m_settings.pos = pos();
    saveSettings();
    QDialog::closeEvent(event);
}

void Window::connectProcess()
{
    connect(oneDriveApp, &Application::processStarted, [this]() {
        eventsInfo(tr("Synchronization restarted"));
    });

    connect(oneDriveApp, &Application::syncComplete, [this]() {
        eventsInfo(tr("Synchronisation completed"));
    });

    connect(oneDriveApp, &Application::localRootDirectoryRemoved, [this]() {
        eventsInfo(tr("The local synchronisation directory was not found"));
    });

    connect(oneDriveApp, &Application::freeSpaceUpdated, [this](uint64_t bytes) {
        eventsInfo(tr("Free space updated to %1 bytes").arg(bytes));
    });

    connect(oneDriveApp, &Application::processStopped, [this]() {
        eventsInfo(tr("Synchronization suspended"));
    });

    connect(oneDriveApp, &Application::fileDeleted, [this](const QString & fileName) {
        eventsOperation(tr("Deleted"), fileName);
    });

    connect(oneDriveApp, &Application::fileRenamed, [this](const QString & from, const QString & to) {
        eventsOperation(tr("Renamed"), tr("'%1' to '%2'").arg(from, to));
    });

    connect(oneDriveApp, &Application::fileUploaded, [this](const QString & fileName) {
        eventsOperation(tr("Uploaded"), fileName);
    });

    connect(oneDriveApp, &Application::fileDownloaded, [this](const QString & fileName) {
        eventsOperation(tr("Downloaded"), fileName);
    });

    connect(oneDriveApp, &Application::localDirectoryCreated, [this](const QString & dirName) {
        eventsOperation(tr("Local directory created"), dirName);
    });

    connect(oneDriveApp, &Application::remoteDirectoryCreated, [this](const QString & dirName) {
        eventsOperation(tr("Remote directory created"), dirName);
    });

    connect(oneDriveApp, &Application::fileDownloaded, [this](const QString & fileName) {
        eventsOperation(tr("Downloaded"), fileName);
    });
}

void Window::eventsInfo(const QString & info)
// Add an info in the recent events
{
    events->appendHtml("<p><span style=\"color:gray;\">" + QDateTime::currentDateTime().toString(QLocale::system().dateFormat(QLocale::ShortFormat)) +
                       " </span><span style=\"color:black;\"> " + info + "</span></p>");
}

void Window::eventsError(const QString & error)
// Add an error in the recent events
{
    events->appendHtml("<p><span style=\"color:gray;\">" + QDateTime::currentDateTime().toString(QLocale::system().dateFormat(QLocale::ShortFormat)) +
                       " </span><span style=\"color:red;\"> " + error + "</span></p>");
}

void Window::eventsOperation(const QString & operation, const QString & fileName)
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

    auto * messageLayout = new QGridLayout;
    messageLayout->addWidget(events, 2, 1, 1, 4);
    messageLayout->setColumnStretch(3, 0);
    messageLayout->setRowStretch(4, 0);

    messageGroupBox = new QGroupBox(tr("Recent events"));
    messageGroupBox->setLayout(messageLayout);

    // ********** Create the baloon warning in the tray ********** //
    //?????? TO DO
    showIconCheckBox = new QCheckBox(tr("Show icon"));
    showIconCheckBox->setChecked(true);
}

void Window::loadSettings()
{
    QSettings settings;
    settings.beginGroup("RecentEventWindow");
    m_settings.size = settings.value("Size", QSize(400, 300)).toSize();
    m_settings.pos = settings.value("Position", QPoint(0, 0)).toPoint();
    settings.endGroup();
}

void Window::saveSettings() const
{
    QSettings settings;
    settings.beginGroup("RecentEventWindow");
    settings.setValue("Size", m_settings.size);
    settings.setValue("Position", m_settings.pos);
    settings.endGroup();
}
