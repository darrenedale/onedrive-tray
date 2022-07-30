#ifndef ONEDRIVETRAY_SETTINGSWINDOW_H
#define ONEDRIVETRAY_SETTINGSWINDOW_H

#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QPlainTextEdit>
#include <QCoreApplication>
#include <QtCore/QtCore>
#include <QtWidgets/QDialog>

QT_BEGIN_NAMESPACE
class QAction;
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

    class SettingsWindow : public QDialog
    {
    Q_OBJECT

    public:
        SettingsWindow();

    protected:
        void closeEvent(QCloseEvent * event) override;

    private slots:

    private:
        void createIconGroupBox();

        void createMessageGroupBox();

        void createActions();

        void createTrayIcon();

        void execute();

        void restart();

        void terminate();

        void createConfigurationGroupBox();

        bool auto_hide;

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
        QLabel * syncdirLabel;
        QPlainTextEdit * terminal;
        QLabel * bodyLabel;
        QComboBox * typeComboBox;
        QSpinBox * durationSpinBox;
        QLineEdit * titleEdit;
        QLineEdit * syncdirEdit;
        QTextEdit * bodyEdit;
        QPushButton * showMessageButton;

        QAction * minimizeAction;
        QAction * maximizeAction;
        QAction * restoreAction;
        QAction * quitAction;
        QAction * consoleAction;
        QAction * configurationAction;
        QAction * restartAction;

        QSystemTrayIcon * trayIcon;
        QMenu * trayIconMenu;
    };
}

#endif
