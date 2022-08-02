/**
 * Application.h
 *
 * Declaration of Application class.
 */

#ifndef ONEDRIVETRAY_APPLICATION_H
#define ONEDRIVETRAY_APPLICATION_H

#include <memory>
#include <stdexcept>
#include <QtCore/QTranslator>
#include <QtWidgets/QApplication>
#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include "IconStyle.h"
#include "Process.h"
#include "MessagesWindow.h"

#define oneDriveApp (dynamic_cast<OneDrive::Application *>(QApplication::instance()))

QT_BEGIN_NAMESPACE
class QMenu;
class QAction;
QT_END_NAMESPACE

namespace OneDrive
{
    using RuntimeException = std::runtime_error;

    /**
     * The application class.
     *
     * The class is a singleton. It reads the m_settings and sets up the UI, starts the onedrive process and waits for
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
        [[nodiscard]] inline const QString & oneDrivePath() const
        {
            return m_oneDrivePath;
        }

        /** Fetch the arguments used to start the onedrive client. */
        [[nodiscard]] inline const QStringList & oneDriveArgs() const
        {
            return m_oneDriveArguments;
        }

        /** Show the application about dialogue. */
        static void showAboutDialogue();

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
         * Fetch whether the application is in debug mode.
         *
         * The application is in debug mode if --debug or -d was passed on the command-line, or it's a debug build.
         *
         * @return `true` if the application is in debug mode, `false` otherwise.
         */
        [[nodiscard]] inline bool inDebugMode() const
        {
            return m_debug;
        }

        /**
         * Fetch a const reference to the tray icon.
         * @return The tray icon.
         */
        [[nodiscard]] inline const QSystemTrayIcon & trayIcon() const
        {
            return m_trayIcon;
        }

        /**
         * Fetch a reference to the tray icon.
         * @return The tray icon.
         */
        [[nodiscard]] inline QSystemTrayIcon & trayIcon()
        {
            return m_trayIcon;
        }

        /**
         * Fetch a const reference to the OneDrive process.
         * @return The process.
         */
        inline const Process & oneDriveProcess() const
        {
            return m_oneDriveProcess;
        }

        /**
         * Fetch a reference to the OneDrive process.
         * @return The process.
         */
        inline Process & oneDriveProcess()
        {
            return m_oneDriveProcess;
        }

        /** Fetch the user's preferred icon style. */
        [[nodiscard]] IconStyle trayIconStyle() const
        {
            return m_settings.iconStyle;
        }

        /** Set the user's preferred icon style. */
        void setTrayIconStyle(IconStyle style);

        /** Load the application m_settings. */
        void loadSettings();

        /** Save the current application m_settings. */
        void saveSettings() const;

        /**
         * Fetch the path to the onedrive client's current config file.
         *
         * @return The path.
         */
        [[nodiscard]] const QString & oneDriveConfigFile() const;

        /** Show the messages window. */
        void showWindow();

        /** Hide the messages window. */
        void hideWindow();

        /** Open the local directory in the user's file manager. */
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

    protected:
        /** Receiver for when the process indicates it has started running. */
        void onProcessStarted();

        /** Receiver for when the process indicates it has stopped running */
        void onProcessStopped();

        /** Receiver for when the process indicates the free space. */
        void onFreeSpaceUpdated(quint64 space);

        /** Receiver for when the process indicates synchronisation has finished. */
        void onSynchronisationComplete();

        /** Receiver for when the process indicates the local root directory has been removed. */
        void onLocalRootDirectoryRemoved();

        /** Receiver for when the process indicates a local directory has been created. */
        void onLocalDirectoryCreated(const QString & directoryName);

        /** Receiver for when the process indicates a remote directory has been created. */
        void onRemoteDirectoryCreated(const QString & directoryName);

        /** Receiver for when the process indicates a file has been deleted. */
        void onFileDeleted(const QString & fileName);

        /** Receiver for when the process indicates a file has been renamed. */
        void onFileRenamed(const QString & from, const QString & to);

        /** Receiver for when the process indicates a file has been uploaded. */
        void onFileUploaded(const QString & fileName);

        /** Receiver for when the process indicates a file has been downloaded. */
        void onFileDownloaded(const QString & fileName);

    private:
        /** Data structure for the application m_settings. */
        struct Settings
        {
            IconStyle iconStyle = IconStyle::colourful;
        };

        /**
         * Helper to expand ~ and $HOME in a path to the full home path.
         *
         * @param path The path to expand.
         *
         * @return The expanded path.
         */
        static QString expandHomeShortcut(const QString & path);

        /** Helper to install the translators. */
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

        /** Update the tray icon based on the current state and the user's prefereed icon style. */
        void refreshTrayIcon();

        /** Helper to populate the tray icon menu. */
        void setupTrayIconMenu();

        /** Helper to connect to signals on the onedrive process. */
        void connectProcess();

        /** The path to the onedrive client. */
        QString m_oneDrivePath;

        /** The args for the onedrive client. */
        QStringList m_oneDriveArguments;

        /** The onedrive process. */
        Process m_oneDriveProcess;

        /** The messages window. */
        MessagesWindow m_window;

        /** The tray icon. */
        QSystemTrayIcon m_trayIcon;

        /**
         * The tray icon menu.
         *
         * Only a member to ensure its lifetime matches the Application instance's.
         */
        QMenu m_trayIconMenu;

        /** The action displaying the current status. */
        QAction m_statusAction;

        /** The action displaying the free OneDrive space. */
        QAction m_freeSpaceAction;

        /** The action to suspend synchronisation. */
        QAction m_suspendAction;

        /** The action to restart synchronisation. */
        QAction m_restartAction;

        /** The application settings. */
        Settings m_settings;

        /** The translator for Qt strings. */
        QTranslator m_qtTranslator;

        /** The translator for application strings. */
        QTranslator m_appTranslator;

        /** Whether the application is in debug mode. */
        bool m_debug;
    };
} // OneDrive

#endif //ONEDRIVETRAY_APPLICATION_H
