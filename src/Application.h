//
// Created by darren on 30/07/22.
//

#ifndef ONEDRIVETRAY_APPLICATION_H
#define ONEDRIVETRAY_APPLICATION_H

#include <stdexcept>
#include <memory>
#include <QApplication>
#include <QTranslator>


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

        void showNotification(const QString & message, int timeout = DefaultNotificationTimeout, NotificationType type = NotificationType::Message);

        inline void showNotification(const QString & message, NotificationType type)
        {
            showNotification(message, DefaultNotificationTimeout, type);
        }

    private:
        void installTranslators();

        QString m_oneDrivePath;
        QString m_oneDriveArgs;
        std::unique_ptr<Window> m_window;

        QTranslator m_qtTranslator;
        QTranslator m_appTranslator;
    };
} // OneDrive

#endif //ONEDRIVETRAY_APPLICATION_H
