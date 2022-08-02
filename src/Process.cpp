/**
 * Process.cpp
 *
 * Implementation of Process class.
 */

#include <iostream>
#include <QtCore/QRegularExpression>
#include "Process.h"
#include "Application.h"

using namespace OneDrive;

namespace
{
    const QString DefaultExecutablePath = QLatin1String("/usr/bin/onedrive");
    const QStringList DefaultArguments = {QLatin1String("--verbose"), QLatin1String("--monitor")};

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


    ProcessMessage parseProcessOutputLine(const QByteArray &line)
    {
        ProcessMessage message;

        if (line.toLower().contains("remaining free space")) {
            message.type = ProcessMessageType::FreeSpace;

            if (auto match = QRegularExpression("([0-9]+)").match(line); match.hasMatch()) {
                message.size = match.captured(1).toLongLong();
            }
        } else if (line.contains("Sync with OneDrive is complete")) {
            message.type = ProcessMessageType::Finished;
        } else if (line.contains("Monitored directory removed")) {
            message.type = ProcessMessageType::LocalRootDirectoryRemoved;
        } else if (line.left(26) == "Creating local directory: ") {
            message.type = ProcessMessageType::CreateLocalDir;
            message.destination = line.right(line.size() - 26);
        } else if (line.left(42) == "Successfully created the remote directory ") {
            message.type = ProcessMessageType::CreateRemoteDir;
            // Remove " on OneDrive" at the end
            message.destination = line.mid(42, line.size() - 54);
        } else if (line.left(7) == "Moving ") {
            message.type = ProcessMessageType::Rename;

            if (const auto match = QRegularExpression("^Moving (.+) to (.+)$").match(line); match.hasMatch()) {
                message.source = match.captured(1);
                message.destination = match.captured(2);
            }
        } else if (auto match = QRegularExpression(
                    "(Downloading (?:new |modified)?file|Uploading (?:new |modified )?file|Deleting item)").match(
                    line); match.hasMatch()) {
            const auto type = match.captured(1);

            if (type == "Deleting item") {
                message.type = ProcessMessageType::Delete;

                if (match = QRegularExpression("(?: item from OneDrive:| item) (.+)$").match(line); match.hasMatch()) {
                    message.destination = match.captured(1);
                }
            } else if (type.contains("Uploading")) {
                message.type = ProcessMessageType::Upload;

                if (match = QRegularExpression(R"((?:Uploading (?:new |modified )?file) (.+) \.\.\.)").match(
                            line); match.hasMatch()) {
                    auto fileName = match.captured(1);

                    if (fileName.endsWith(" ...")) {
                        fileName.truncate(fileName.size() - 4);
                    }

                    message.destination = fileName;
                }
            } else if (line.contains("Downloading")) {
                message.type = ProcessMessageType::Download;

                if (match = QRegularExpression(R"((?:Downloading (?:new |modified )?file) (.+) \.\.\.)").match(
                            line); match.hasMatch()) {
                    auto fileName = match.captured(1);

                    if (fileName.endsWith(" ...")) {
                        fileName.truncate(fileName.size() - 4);
                    }

                    message.destination = fileName;
                }
            }
        }

        return message;
    }
}


Process::Process(const std::optional<QString> &executable, const std::optional<QStringList> &args)
        : QProcess(),
          m_executablePath(static_cast<bool>(executable) ? *executable : DefaultExecutablePath),
          m_args(static_cast<bool>(args) ? *args : DefaultArguments),
          m_syncState(SynchronisationState::Idle)
{
    connect(this, &QProcess::readyReadStandardOutput, this, &Process::readOutput);
    connect(this, &QProcess::readyReadStandardError, this, &Process::readError);
    connect(this, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &Process::stopped);
}


Process::~Process() = default;


void Process::readOutput()
{
    static QByteArray buffer;

    buffer += readAllStandardOutput();

    if (oneDriveApp->inDebugMode()) {
        std::cerr << qPrintable(buffer) << std::flush;
    }

    for (const QByteArray &line: buffer.split('\n')) {
        const auto previousSyncState = synchronisationState();
        const auto message = parseProcessOutputLine(line);

        switch (message.type) {
            case ProcessMessageType::Unknown:
                m_syncState = SynchronisationState::Idle;
                break;

            case ProcessMessageType::FreeSpace:
                m_syncState = SynchronisationState::Idle;
                Q_EMIT freeSpaceUpdated(message.size);
                Q_EMIT synchronisationComplete();
                break;

            case ProcessMessageType::Finished:
                m_syncState = SynchronisationState::Idle;
                Q_EMIT synchronisationComplete();
                break;

            case ProcessMessageType::LocalRootDirectoryRemoved:
                m_syncState = SynchronisationState::Idle;
                Q_EMIT localRootDirectoryRemoved();
                break;

            case ProcessMessageType::CreateLocalDir:
                m_syncState = SynchronisationState::Syncing;
                Q_EMIT localDirectoryCreated(message.destination);
                break;

            case ProcessMessageType::CreateRemoteDir:
                m_syncState = SynchronisationState::Syncing;
                Q_EMIT remoteDirectoryCreated(message.destination);
                break;

            case ProcessMessageType::Delete:
                m_syncState = SynchronisationState::Syncing;
                Q_EMIT fileDeleted(message.destination);
                break;

            case ProcessMessageType::Rename:
                m_syncState = SynchronisationState::Syncing;
                Q_EMIT fileRenamed(message.source, message.destination);
                break;

            case ProcessMessageType::Upload:
                m_syncState = SynchronisationState::Syncing;
                Q_EMIT fileUploaded(message.destination);
                break;

            case ProcessMessageType::Download:
                m_syncState = SynchronisationState::Syncing;
                Q_EMIT fileDownloaded(message.destination);
                break;
        }

        if (previousSyncState != synchronisationState()) {
            Q_EMIT synchronisationStateChanged(synchronisationState(), previousSyncState);
        }
    }

    if (const int lastLineEnd = buffer.lastIndexOf('\n'); 0 <= lastLineEnd) {
        buffer = buffer.right(buffer.size() - lastLineEnd);
    } else {
        buffer.clear();
    }
}


void Process::readError()
{
    const auto errors = readAllStandardError();

    if (!oneDriveApp->inDebugMode()) {
        return;
    }

    std::cerr << qPrintable(errors) << std::flush;
}
