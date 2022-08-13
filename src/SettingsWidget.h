#ifndef ONEDRIVE_SETTINGSWIDGET_H
#define ONEDRIVE_SETTINGSWIDGET_H

#include <memory>
#include <QtWidgets/QWidget>

namespace Ui
{
    class SettingsWidget;
}

namespace OneDrive
{
    class Settings;

    class SettingsWidget
            : public QWidget
    {
    Q_OBJECT

    public:
        SettingsWidget(const Settings & settings, QWidget *parent = nullptr);

        ~SettingsWidget() override;

        [[nodiscard]] bool startOwnOneDrive() const;
        void setStartOwnOneDrive(bool start);

        [[nodiscard]] bool useCustomOneDrive() const;
        void setUseCustomOneDrive(bool use);

        [[nodiscard]] bool useCustomSocket() const;
        void setUseCustomSocket(bool use);

        [[nodiscard]] QString customOneDrivePath() const;
        void setCustomOneDrivePath(const QString & exe);

        [[nodiscard]] QString customSocketPath() const;
        void setCustomSocketPath(const QString & socketFile);

    Q_SIGNALS:
        void changed() const;

    protected:
        void synchroniseWidgetStates();

    private:
        void connectComponents();

        std::unique_ptr<Ui::SettingsWidget> m_ui;
    };
}

#endif // ONEDRIVE_SETTINGSWIDGET_H
