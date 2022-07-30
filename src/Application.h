//
// Created by darren on 30/07/22.
//

#ifndef ONEDRIVETRAY_APPLICATION_H
#define ONEDRIVETRAY_APPLICATION_H

#include <stdexcept>
#include <memory>
#include <QApplication>
#include <QTranslator>
#include "IconStyle.h"
#include "SyncState.h"

#define oneDriveApp (dynamic_cast<OneDrive::Application *>(QApplication::instance()))

QT_BEGIN_NAMESPACE
class QSystemTrayIcon;
QT_END_NAMESPACE

namespace OneDrive
{
    class Window;

    using RuntimeException = std::runtime_error;

    class Application
            : public QApplication
    {
    Q_OBJECT

    public:
        enum class NotificationType
        {
            Message,
            Warning,
            Error,
        };

        static const int DefaultNotificationTimeout = 5000;
        static const int NoNotificationTimeout = 0;

        Application(int & argc, char ** argv);

        ~Application() noexcept override;

        [[nodiscard]] const QString & oneDrivePath() const;

        [[nodiscard]] const QString & oneDriveArgs() const;

        void showAboutDialogue() const;

        void showNotification(const QString & message, int timeout = DefaultNotificationTimeout, NotificationType type = NotificationType::Message);

        inline void showNotification(const QString & message, NotificationType type)
        {
            showNotification(message, DefaultNotificationTimeout, type);
        }

        void openLocalDirectory() const;

        inline QSystemTrayIcon & trayIcon() const
        {
            return *m_trayIcon;
        }

        inline SyncState state() const
        {
            return m_state;
        }

        IconStyle iconStyle() const;
        void setTrayIconStyle(IconStyle style);

        void loadSettings();
        void saveSettings() const;

        const QString & oneDriveConfigFile() const;

    private:
        struct Settings
        {
            IconStyle iconStyle = IconStyle::colourful;
        };

        QString determineOneDriveConfigFile() const;

        void refreshTrayIcon();
        void createTrayIconMenu();
        void installTranslators();

        SyncState m_state;
        Settings settings;
        QString m_oneDrivePath;
        QString m_oneDriveArgs;
        std::unique_ptr<Window> m_window;
        std::unique_ptr<QSystemTrayIcon> m_trayIcon;

        QTranslator m_qtTranslator;
        QTranslator m_appTranslator;
    };
} // OneDrive

#endif //ONEDRIVETRAY_APPLICATION_H
