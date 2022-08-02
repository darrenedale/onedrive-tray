#ifndef ONEDRIVETRAY_MESSAGESWINDOW_H
#define ONEDRIVETRAY_MESSAGESWINDOW_H

#include <QtCore/QSize>
#include <QtCore/QPoint>
#include <QtWidgets/QDialog>

QT_BEGIN_NAMESPACE
class QString;
class QGroupBox;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace OneDrive
{
    class MessagesWindow : public QDialog
    {
    Q_OBJECT

    public:
        MessagesWindow();
        ~MessagesWindow() override;

    protected:
        void closeEvent(QCloseEvent * event) override;

    private:
        void connectProcess();

        void createMessageGroupBox();

        void loadSettings();

        void saveSettings() const;

        void addInfoMessage(const QString & info);

        void addErrorMessage(const QString & error);

        void addOperationMessage(const QString & Operation, const QString & fileName);

        struct WindowSettings
        {
            QSize size;
            QPoint pos;
        };

        QGroupBox * m_messagesContainer;
        QPlainTextEdit * m_eventsList;
        WindowSettings m_settings;
    };
}

#endif
