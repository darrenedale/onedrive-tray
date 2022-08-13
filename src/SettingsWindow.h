//
// Created by darren on 13/08/22.
//

#ifndef ONEDRIVETRAY_SETTINGSWINDOW_H
#define ONEDRIVETRAY_SETTINGSWINDOW_H

#include <memory>
#include <QtWidgets/QDialog>

namespace OneDrive
{
    class Settings;
    class SettingsWidget;

    class SettingsWindow
    : public QDialog
    {
    public:
        SettingsWindow(const Settings & settings, QWidget * parent = nullptr);
        ~SettingsWindow() noexcept override;

        [[nodiscard]] SettingsWidget * settingsWidget() const
        {
            return m_settingsWidget.get();
        }

    private:
        std::unique_ptr<SettingsWidget> m_settingsWidget;
    };

} // OneDrive

#endif //ONEDRIVETRAY_SETTINGSWINDOW_H
