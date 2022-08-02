/**
 * Process.h
 *
 * Declaration of Process class.
 */

#ifndef ONEDRIVETRAY_PROCESS_H
#define ONEDRIVETRAY_PROCESS_H

#include <optional>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace OneDrive
{

    class Process
            : public QProcess
    {
    Q_OBJECT

    public:
        enum class SynchronisationState
        {
            Idle = 0,
            Syncing,
        };

        explicit Process(const std::optional<QString> &executable = {}, const std::optional<QStringList> &args = {});

        ~Process() override;

        [[nodiscard]] inline bool isRunning() const
        {
            return QProcess::ProcessState::NotRunning != state();
        }

        [[nodiscard]] inline SynchronisationState synchronisationState() const
        {
            return m_syncState;
        }

    Q_SIGNALS:

        /** Emitted when the onedrive process has stopped/been suspended. */
        void stopped();

        /** Emitted when the onedrive process has completed its current sync. */
        void synchronisationComplete();

        /** Emitted if the onedrive process indicates it has detected the local root sync directory is missing. */
        void localRootDirectoryRemoved();

        /** Emitted when the onedrive process outputs the free space on the OneDrive. */
        void freeSpaceUpdated(uint64_t bytes);

        /** Emitted when the onedrive process uploads a file. */
        void fileUploaded(const QString &fileName);

        /** Emitted when the onedrive process downloads a file. */
        void fileDownloaded(const QString &fileName);

        /** Emitted when the onedrive process creates a local directory. */
        void localDirectoryCreated(const QString &dirName);

        /** Emitted when the onedrive process creates a remote directory. */
        void remoteDirectoryCreated(const QString &dirName);

        /** Emitted when the onedrive process renames a file. */
        void fileRenamed(const QString &from, const QString &to);

        /** Emitted when the onedrive process deletes a file. */
        void fileDeleted(const QString &fileName);

        void synchronisationStateChanged(SynchronisationState to, SynchronisationState from) const;

    protected:
        void readOutput();
        void readError();

    private:
        QString m_executablePath;
        QStringList m_args;
        SynchronisationState m_syncState;
    };

} // OneDrive

#endif //ONEDRIVETRAY_PROCESS_H
