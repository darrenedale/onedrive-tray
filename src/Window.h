#ifndef ONEDRIVETRAY_WINDOW_H
#define ONEDRIVETRAY_WINDOW_H

#include <QtCore/QSize>
#include <QtCore/QPoint>
#include <QtWidgets/QDialog>

QT_BEGIN_NAMESPACE
class QString;
class QCheckBox;
class QGroupBox;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace OneDrive
{
    class Window : public QDialog
    {
    Q_OBJECT

    public:
        Window();

    protected:
        void closeEvent(QCloseEvent * event) override;

    private:
        void connectProcess();
        void createIconGroupBox();

        void createMessageGroupBox();

        void loadSettings();

        void saveSettings() const;

        void eventsInfo(const QString & info);

        void eventsError(const QString & error);

        void eventsOperation(const QString & Operation, const QString & fileName);

        struct WindowSettings
        {
            QSize size;
            QPoint pos;
        };

        QCheckBox * showIconCheckBox;
        QGroupBox * messageGroupBox;
        QPlainTextEdit * events;
        WindowSettings m_settings;
    };
}

#endif
