
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
: QDialog()
{
    // Used to show the window in odd clicks and hide in even.
    auto_hide = true;

    // Load the settings of the application
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

    eventsInfo(tr("OneDrive started"));
}

void Window::suspend()
{
    m_oneDriveProcess->terminate(); // Kill m_oneDriveProcess.
    m_oneDriveProcess->waitForFinished(); // Wait for m_oneDriveProcess finish
    restartAction->setVisible(true);
    suspendAction->setVisible(false);
    refreshTrayIcon(true, false);

    statusAction->setText(tr("Synchronization suspended"));
    eventsInfo(tr("Synchronization suspended"));
}

void Window::restart()
{
    m_oneDriveProcess->start(); // Start the m_oneDriveProcess.
    suspendAction->setVisible(true);
    restartAction->setVisible(false);

    eventsInfo(tr("Synchronization restarted"));
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

void Window::quit()
{
    m_oneDriveProcess->terminate();
    saveSettings();
    qApp->quit();
}

void Window::iconActivated(QSystemTrayIcon::ActivationReason reason)
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
            // Get m_oneDriveProcess identifier (PID).
            qint64 pid = m_oneDriveProcess->processId();

            // Create the string to store the informations.
            QString text;

            // If pid is greater than 0 the m_oneDriveProcess is running
            // and when show a info about this.
            if (pid > 0) {
                text.append(tr("OneDrive is running with the PID "));
                text.append(QString::number(pid));
                text.append(".\n");
            }
                // If not the m_oneDriveProcess is stopped and when notify the user.
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

void Window::saveSettings()
{
    QSettings settings;
    settings.beginGroup("RecentEventWindow");
    settings.setValue("Size", m_settings.size);
    settings.setValue("Position", m_settings.pos);
    settings.endGroup();
}
