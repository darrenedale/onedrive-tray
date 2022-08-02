//
// Created by darren on 30/07/22.
//

#ifndef ONEDRIVETRAY_APPLICATION_H
#define ONEDRIVETRAY_APPLICATION_H

#include <memory>
#include <stdexcept>
#include <QtCore/QTranslator>
#include <QtWidgets/QApplication>
#include <QtWidgets/QSystemTrayIcon>
#include "IconStyle.h"
#include "SyncState.h"

#define oneDriveApp (dynamic_cast<OneDrive::Application *>(QApplication::instance()))

QT_BEGIN_NAMESPACE
class QMenu;
class QAction;
class QProcess;
QT_END_NAMESPACE

namespace OneDrive
{
    class MessagesWindow;

    using RuntimeException = std::runtime_error;

    /**
     * The application class.
     *
     * The class is a singleton. It reads the settings and sets up the UI, starts the onedrive process and waits for
     * user input and messages from the client. It emits signals when the client indicates something of note has
     * happened.
     */
    class Application
            : public QApplication
    {
    Q_OBJECT

    public:
        /** Enumeration of the types of notification the app can display. */
        enum class NotificationType
        {
            Message,
            Warning,
            Error,
        };

        /** The defualt duration (in ms) for which notifications will be displayed.*/
        static const int DefaultNotificationTimeout = 5000;

        /** Use this to specify that a notification should not time out. */
        static const int NoNotificationTimeout = 0;

        /**
         * Initialise a new Application instance.
         *
         * @param argc A reference to the argument count.
         * @param argv The arguments as an array of c-style strings. Must be at least `argc` strings.
         */
        Application(int & argc, char ** argv);

        /** Destructor. */
        ~Application() noexcept override;

        /** Fetch the path to the onedrive client. */
        [[nodiscard]] const QString & oneDrivePath() const;

        /** Fetch the arguments used to start the onedrive client. */
        [[nodiscard]] const QString & oneDriveArgs() const;

        /** Show the application about dialogue. */
        void showAboutDialogue() const;

        /**
         * Show a notification to the user.
         *
         * @param message The message to show.
         * @param timeout How many ms to show the notification for.
         * @param type The notification type.
         */
        void showNotification(const QString & message, int timeout = DefaultNotificationTimeout, NotificationType type = NotificationType::Message) const;

        /**
         * Show a notification to the user.
         *
         * @param message The message to show.
         * @param type The notification type.
         */
        inline void showNotification(const QString & message, NotificationType type) const
        {
            showNotification(message, DefaultNotificationTimeout, type);
        }

        /**
         * Fetch a reference to the tray icon.
         * @return
         */
        inline QSystemTrayIcon & trayIcon() const
        {
            return *m_trayIcon;
        }

        /**
         * Fetch the current sync state.
         *
         * @return The state.
         */
        inline SyncState state() const
        {
            return m_state;
        }

        /** Fetch the user's preferred icon style. */
        [[nodiscard]] IconStyle iconStyle() const;

        /** Set the user's preferred icon style. */
        void setTrayIconStyle(IconStyle style);

        /** Load the application settings. */
        void loadSettings();

        /** Save the current application settings. */
        void saveSettings() const;

        /**
         * Fetch the path to the onedrive client's current config file.
         *
         * @return The path.
         */
        [[nodiscard]] const QString & oneDriveConfigFile() const;

        /** Suspend the onedrive process. */
        void suspendProcess();

        /** (Re)start the onedrive process. */
        void restartProcess();

        /**
         * Show the messages window.
         */
        void showWindow();

        /**
         * Hide the messages window.
         */
        void hideWindow();

        /**
         * Open the local directory in the user's file manager.
         */
        void openLocalDirectory() const;

        /**
         * Run the application.
         *
         * Show the icon, start the onedrive client process, and wait for it to send output.
         *
         * @return The exit code.
         */
        int exec();

    Q_SIGNALS:
        /** Emitted when the onedrive process has stopped/been suspsended. */
        void processStopped();

        /** Emitted when the onedrive process has (re)started. */
        void processStarted();

        /** Emitted when the onedrive process has completed its current sync. */
        void syncComplete();

        /** Emitted if the onedrive process indicates it has detected the local root sync directory is missing. */
        void localRootDirectoryRemoved();

        /** Emitted when the onedrive process outputs the free space on the OneDrive. */
        void freeSpaceUpdated(uint64_t bytes);

        /** Emitted when the onedrive process uploads a file. */
        void fileUploaded(const QString & fileName);

        /** Emitted when the onedrive process downloads a file. */
        void fileDownloaded(const QString & fileName);

        /** Emitted when the onedrive process creates a local directory. */
        void localDirectoryCreated(const QString & dirName);

        /** Emitted when the onedrive process creates a remote directory. */
        void remoteDirectoryCreated(const QString & dirName);

        /** Emitted when the onedrive process renames a file. */
        void fileRenamed(const QString & from, const QString & to);

        /** Emitted when the onedrive process deletes a file. */
        void fileDeleted(const QString & fileName);

    private:
        /** Data structure for the application settings. */
        struct Settings
        {
            IconStyle iconStyle = IconStyle::colourful;
        };

        /** Enumeration of the types of message that can be parsed from the onedrive client output. */
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

        /** A parsed message from the onedrive client. */
        struct ProcessMessage
        {
            ProcessMessageType type = ProcessMessageType::Unknown;
            uint64_t size = 0;
            QString source;
            QString destination;
        };

        /**
         * Helper to expand ~ and $HOME in a path to the full home path.
         *
         * @param path The path to expand.
         *
         * @return The expanded path.
         */
        static QString expandHomeShortcut(const QString & path);

        /**
         * Helper to install the translators.
         */
        void installTranslators();

        /**
         * Helper to locate the config file onedrive is using.
         *
         * @return The path to the config file.
         */
        [[nodiscard]] QString locateOneDriveConfigFile() const;

        /**
         * Slot for when the tray icon is activated.
         *
         * @param reason The activation reason.
         */
        void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

        /**
         * Update the tray icon based on the current state and the user's prefereed icon style.
         */
        void refreshTrayIcon();

        /**
         * Helper to populate the tray icon menu.
         */
        void createTrayIconMenu();

        /**
         * Helper to parse a line of standard output from the onedrive process.
         *
         * @param line The line read.
         *
         * @return The parsed message.
         */
        static ProcessMessage parseProcessOutputLine(const QByteArray & line);

        /**
         * Slot to read the output of the onedrive process.
         */
        void readProcessOutput();

        /**
         * Slot to read the error output from the onedrive process.
         */
        void readProcessError();

        /** The current onedrive process state. */
        SyncState m_state;

        /** The application settings. */
        Settings settings;

        /** The path to the onedrive client. */
        QString m_oneDrivePath;

        /** The args for the onedrive client. */
        QString m_oneDriveArgs;

        /** The messages window. */
        std::unique_ptr<MessagesWindow> m_window;

        /** The tray icon. */
        std::unique_ptr<QSystemTrayIcon> m_trayIcon;

        /**
         * The tray icon menu.
         *
         * Only a member to ensure its lifetime matches the Application instance's.
         */
        std::unique_ptr<QMenu> m_trayIconMenu;

        /** The action displaying the current status. */
        std::unique_ptr<QAction> m_statusAction;

        /** The action displaying the free OneDrive space. */
        std::unique_ptr<QAction> m_freeSpaceAction;

        /** The action to suspend synchronisation. */
        std::unique_ptr<QAction> m_suspendAction;

        /** The action to restart synchronisation. */
        std::unique_ptr<QAction> m_restartAction;

        /** The onedrive process. */
        std::unique_ptr<QProcess> m_oneDriveProcess;

        QTranslator m_qtTranslator;
        QTranslator m_appTranslator;
    };
} // OneDrive

#endif //ONEDRIVETRAY_APPLICATION_H
