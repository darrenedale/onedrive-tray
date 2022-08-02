/**
 * MessagesWindow.cpp
 *
 * Implementation of MessagesWindow class.
 */

#include <stdexcept>
#include <QtCore/QDateTime>
#include <QtCore/QSettings>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QComboBox>
#include <QtGui/QCloseEvent>
#include <QtCore/QDebug>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include "MessagesWindow.h"
#include "Application.h"

using namespace OneDrive;

MessagesWindow::MessagesWindow(const Process & process)
: QDialog(),
  m_process(process),
  m_messagesContainer(nullptr),
  m_eventsList(nullptr)
{
    loadSettings();
    createMessageGroupBox();

    auto * mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_messagesContainer);
    setLayout(mainLayout);

    setWindowTitle(tr("Recent m_eventsList"));
    resize(m_settings.size);

    if (!m_settings.pos.isNull()) {
        move(m_settings.pos);
    }

    connectProcess();
    addInfoMessage(tr("OneDrive started"));
}

MessagesWindow::~MessagesWindow() = default;

void MessagesWindow::closeEvent(QCloseEvent * event)
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

void MessagesWindow::connectProcess()
{
    connect(&m_process, &Process::started, [this] () {
        addInfoMessage(tr("Synchronization restarted"));
    });

    connect(&m_process, &Process::synchronisationComplete, [this]( ) {
        addInfoMessage(tr("Synchronisation completed"));
    });

    connect(&m_process, &Process::localRootDirectoryRemoved, [this] () {
        addInfoMessage(tr("The local synchronisation directory was not found"));
    });

    connect(&m_process, &Process::freeSpaceUpdated, [this] (uint64_t bytes) {
        addInfoMessage(tr("Free space updated to %1 bytes").arg(bytes));
    });

    connect(&m_process, &Process::stopped, [this] () {
        addInfoMessage(tr("Synchronization suspended"));
    });

    connect(&m_process, &Process::fileDeleted, [this] (const QString & fileName) {
        addOperationMessage(tr("Deleted"), fileName);
    });

    connect(&m_process, &Process::fileRenamed, [this] (const QString & from, const QString & to) {
        addOperationMessage(tr("Renamed"), tr("'%1' to '%2'").arg(from, to));
    });

    connect(&m_process, &Process::fileUploaded, [this] (const QString & fileName) {
        addOperationMessage(tr("Uploaded"), fileName);
    });

    connect(&m_process, &Process::fileDownloaded, [this] (const QString & fileName) {
        addOperationMessage(tr("Downloaded"), fileName);
    });

    connect(&m_process, &Process::localDirectoryCreated, [this] (const QString & dirName) {
        addOperationMessage(tr("Local directory created"), dirName);
    });

    connect(&m_process, &Process::remoteDirectoryCreated, [this] (const QString & dirName) {
        addOperationMessage(tr("Remote directory created"), dirName);
    });

    connect(&m_process, &Process::fileDownloaded, [this] (const QString & fileName) {
        addOperationMessage(tr("Downloaded"), fileName);
    });
}

void MessagesWindow::addInfoMessage(const QString & info)
{
    m_eventsList->appendHtml("<p><span style=\"color:gray;\">" + QDateTime::currentDateTime().toString(QLocale::system().dateFormat(QLocale::ShortFormat)) +
                             " </span><span style=\"color:black;\"> " + info + "</span></p>");
}

void MessagesWindow::addErrorMessage(const QString & error)
{
    m_eventsList->appendHtml("<p><span style=\"color:gray;\">" + QDateTime::currentDateTime().toString(QLocale::system().dateFormat(QLocale::ShortFormat)) +
                             " </span><span style=\"color:red;\"> " + error + "</span></p>");
}

void MessagesWindow::addOperationMessage(const QString & operation, const QString & fileName)
{
    m_eventsList->appendHtml("<p><span style=\"color:gray;\">" + QDateTime::currentDateTime().toString(QLocale::system().dateFormat(QLocale::ShortFormat)) +
                             "</span><span style=\"color:blue;\"> " + operation + ", <b>" + fileName + "</b> </span></p>");
}

void MessagesWindow::createMessageGroupBox()
{
    m_eventsList = new QPlainTextEdit(this);
    m_eventsList->setReadOnly(true);
    m_eventsList->setMaximumBlockCount(5000);

    auto * messageLayout = new QGridLayout(this);
    messageLayout->addWidget(m_eventsList, 2, 1, 1, 4);
    messageLayout->setColumnStretch(3, 0);
    messageLayout->setRowStretch(4, 0);

    m_messagesContainer = new QGroupBox(tr("Recent m_eventsList"), this);
    m_messagesContainer->setLayout(messageLayout);
}

void MessagesWindow::loadSettings()
{
    QSettings settings;
    settings.beginGroup("RecentEventWindow");
    m_settings.size = settings.value("Size", QSize(400, 300)).toSize();
    m_settings.pos = settings.value("Position", QPoint(0, 0)).toPoint();
    settings.endGroup();
}

void MessagesWindow::saveSettings() const
{
    QSettings settings;
    settings.beginGroup("RecentEventWindow");
    settings.setValue("Size", m_settings.size);
    settings.setValue("Position", m_settings.pos);
    settings.endGroup();
}
