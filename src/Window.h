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

        void showMessage(const QString & text);

        void readStdOutput();

        void readStdError();

        void setTrayIconStyle(const IconStyle &);

    private:
        void createIconGroupBox();

        void createMessageGroupBox();

        void createActions();

        void createTrayIcon();

        void execute(QString onedrive_path, QString onedrive_arguments);

        void openFolder();

        void restart();

        void suspend();

        void refreshTrayIcon(bool forceChange, bool sync);

        void quit();

        void about();

        void loadSettings();

        void saveSettings();

        void eventsInfo(QString info);

        void eventsError(QString error);

        void eventsOperation(QString Operation, QString fileName);

        struct AppConfiguration
        {
            IconStyle iconStyle = IconStyle::colourful;
            QSize size;
            QPoint pos;
        };

        bool auto_hide;
        bool isSyncing;

        QCheckBox * showIconCheckBox;
        QProcess * process;

        QGroupBox * messageGroupBox;
        QPlainTextEdit * events;
        QString * stdOutputString;

        QAction * freeSpaceAction;
        QAction * statusAction;
        QAction * consoleAction;
        QAction * openfolderAction;
        QAction * restartAction;
        QAction * suspendAction;
        QActionGroup * iconColorGroup;
        QAction * quitAction;
        QAction * aboutAction;

        QSystemTrayIcon * trayIcon;
        QMenu * trayIconMenu;

        AppConfiguration appConfig;
    };
}

#endif
