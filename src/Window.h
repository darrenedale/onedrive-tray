#ifndef WINDOW_H
#define WINDOW_H

#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QPlainTextEdit>
#include <QtCore/QCoreApplication>
#include <QtCore/QtCore>
#include <QtWidgets/QDialog>
#include "IconStyle.h"

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;
class QCheckBox;
class QGroupBox;
class QMenu;
QT_END_NAMESPACE

namespace OneDrive
{
    class Window : public QDialog
    {
    Q_OBJECT

    public:
        Window();

    protected:
        void closeEvent(QCloseEvent * event) override;

    private slots:

        void iconActivated(QSystemTrayIcon::ActivationReason reason);

    private:
        void createIconGroupBox();

        void createMessageGroupBox();

        void restart();

        void suspend();

        void refreshTrayIcon(bool forceChange, bool sync);

        void quit();

        void loadSettings();

        void saveSettings();

        void eventsInfo(QString info);

        void eventsError(QString error);

        void eventsOperation(QString Operation, QString fileName);

        struct WindowSettings
        {
            QSize size;
            QPoint pos;
        };

        bool auto_hide;

        QCheckBox * showIconCheckBox;
        QProcess * m_oneDriveProcess;

        QGroupBox * messageGroupBox;
        QPlainTextEdit * events;

        QAction * statusAction;
        QAction * restartAction;
        QAction * suspendAction;

        WindowSettings m_settings;
    };
}

#endif
