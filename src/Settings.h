//
// Created by darren on 13/08/22.
//

#ifndef ONEDRIVETRAY_SETTINGS_H
#define ONEDRIVETRAY_SETTINGS_H

#include "IconStyle.h"
#include <string>

namespace OneDrive
{

    class Settings
    {
        public:
        Settings() = default;
        virtual ~Settings() = default;

        [[nodiscard]] const IconStyle & iconStyle() const
        {
            return m_iconStyle;
        }

        void setIconStyle(const IconStyle & style)
        {
            m_iconStyle = style;
        }

        [[nodiscard]] bool startOwnOneDrive() const
        {
            return m_startOwnOneDrive;
        }

        void setStartOwnOneDrive(bool start)
        {
            m_startOwnOneDrive = start;
        }

        [[nodiscard]] bool useCustomOneDrive() const
        {
            return m_useCustomOneDrive;
        }

        void setUseCustomOneDrive(bool use)
        {
            m_useCustomOneDrive = use;
        }

        [[nodiscard]] const std::string & customOneDrivePath() const
        {
            return m_oneDrivePath;
        }

        void setCustomOneDrivePath(std::string && path)
        {
            m_oneDrivePath = std::move(path);
        }

        [[nodiscard]] bool useCustomSocket() const
        {
            return m_useCustomSocket;
        }

        void setUseCustomSocket(bool use)
        {
            m_useCustomSocket = use;
        }

        [[nodiscard]] const std::string & customSocketPath() const
        {
            return m_socketPath;
        }

        void setCustomSocketPath(std::string && path)
        {
            m_socketPath = std::move(path);
        }

    private:
        IconStyle m_iconStyle;
        bool m_startOwnOneDrive;
        bool m_useCustomOneDrive;
        std::string m_oneDrivePath;
        bool m_useCustomSocket;
        std::string m_socketPath;
    };

} // OneDrive

#endif //ONEDRIVETRAY_SETTINGS_H
