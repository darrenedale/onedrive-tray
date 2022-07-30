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
class QMenu;
class QAction;
class QProcess;
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

        void showNotification(const QString & message, int timeout = DefaultNotificationTimeout, NotificationType type = NotificationType::Message) const;

        inline void showNotification(const QString & message, NotificationType type) const
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

        void openLocalDirectory() const;
        int exec();

    Q_SIGNALS:
        void syncComplete();
        void localRootDirectoryRemoved();
        void freeSpaceUpdated(uint64_t bytes);
        void fileUploaded(const QString & fileName);
        void fileDownloaded(const QString & fileName);
        void localDirectoryCreated(const QString & dirName);
        void remoteDirectoryCreated(const QString & dirName);
        void fileRenamed(const QString & from, const QString & to);
        void fileDeleted(const QString & fileName);

    private:
        struct Settings
        {
            IconStyle iconStyle = IconStyle::colourful;
        };

        enum class ProcessMessageType
        {
            Unknown = 0,
            FreeSpace,
            Finished,
            LocalRootDirectoryRemoved,
            CreateLocalDir,
            CreateRemoteDir,
            Rename,
            Delete,
            Upload,
            Download,
        };

        struct ProcessMessage
        {
            ProcessMessageType type = ProcessMessageType::Unknown;
            uint64_t size = 0;
            QString source;
            QString destination;
        };

        static ProcessMessage parseProcessOutputLine(const QByteArray & line);

        QString locateOneDriveConfigFile() const;

        static QString expandHomeShortcut(const QString & path);

        void refreshTrayIcon();
        void createTrayIconMenu();
        void installTranslators();
        void readProcessOutput();
        void readProcessError();

        SyncState m_state;
        Settings settings;
        QString m_oneDrivePath;
        QString m_oneDriveArgs;
        std::unique_ptr<Window> m_window;
        std::unique_ptr<QSystemTrayIcon> m_trayIcon;
        std::unique_ptr<QMenu> m_trayIconMenu;
        std::unique_ptr<QAction> m_statusAction;
        std::unique_ptr<QAction> m_freeSpaceAction;
        std::unique_ptr<QProcess> m_oneDriveProcess;

        QTranslator m_qtTranslator;
        QTranslator m_appTranslator;
    };
} // OneDrive

#endif //ONEDRIVETRAY_APPLICATION_H
