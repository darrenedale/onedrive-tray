#ifndef WINDOW_H
#define WINDOW_H

#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QPlainTextEdit>
#include <QtCore/QCoreApplication>
#include <QtCore/QtCore>
#include <QtWidgets/QDialog>
#include "SettingsWindow.h"

QT_BEGIN_NAMESPACE
class QAction;

class QActionGroup;

class QCheckBox;

class QComboBox;

class QGroupBox;

class QLabel;

class QLineEdit;

class QMenu;

class QPushButton;

class QSpinBox;

class QTextEdit;

QT_END_NAMESPACE

namespace OneDrive
{
    class Window : public QDialog
    {
    Q_OBJECT

    public:
        Window(QString onedrive_path, QString onedrive_arguments);

        QString * arguments;
        QString * path;

    protected:
        void closeEvent(QCloseEvent * event) override;

    private slots:

        void iconActivated(QSystemTrayIcon::ActivationReason reason);

        void showMessage(const QString & text);

        void readStdOutput();

        void readStdError();

        void OpenConfigurationWindow();

        void defineTrayIcon(const QColor & color);

        void moreColors();

    private:
        void createIconGroupBox();

        void createMessageGroupBox();

        void createActions();

        void createTrayIcon();

        void execute(QString onedrive_path, QString onedrive_arguments);

        void openFolder();

        void restart();

        void suspend();

        void changeTrayIcon(bool forceChange, bool sync);

        void quit();

        void about();

        void createConfigurationGroupBox();

        void loadSettings();

        void saveSettings();

        void eventsInfo(QString info);

        void eventsError(QString error);

        void eventsOperation(QString Operation, QString fileName);

        struct AppConfiguration
        {
            QColor iconColor;
            QSize size;
            QPoint pos;
        };

        bool auto_hide;
        bool isSyncing;

        QGroupBox * iconGroupBox;
        QLabel * iconLabel;
        QComboBox * iconComboBox;
        QCheckBox * showIconCheckBox;
        QProcess * process;

        QGroupBox * messageGroupBox;
        QLabel * typeLabel;
        QLabel * durationLabel;
        QLabel * durationWarningLabel;
        QLabel * titleLabel;
        QPlainTextEdit * events;
        QString * stdOutputString;
        QLabel * bodyLabel;
        QComboBox * typeComboBox;
        QSpinBox * durationSpinBox;
        QLineEdit * titleEdit;
        QTextEdit * bodyEdit;
        QPushButton * showMessageButton;

        QAction * freeSpaceAction;
        QAction * statusAction;
        QAction * consoleAction;
        QAction * configurationAction;
        QAction * openfolderAction;
        QAction * restartAction;
        QAction * suspendAction;
        QActionGroup * iconColorGroup;
        QAction * quitAction;
        QAction * aboutAction;

        QSystemTrayIcon * trayIcon;
        QMenu * trayIconMenu;
        QString * currentIconPath;

        SettingsWindow * ConfigurationWindow;

        AppConfiguration * appConfig;
    };
}

#endif
