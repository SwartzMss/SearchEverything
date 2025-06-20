#include "searchworker.h"
#include <QFileInfo>

SearchWorker::SearchWorker(QObject *parent) : QObject(parent)
{
    connect(&process, &QProcess::readyReadStandardOutput, this, &SearchWorker::onReadyRead);
    connect(&process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &SearchWorker::finished);
}

void SearchWorker::start(const QString &rgExePath, const QStringList &arguments)
{
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(rgExePath, arguments);
}

void SearchWorker::stop()
{
    if (process.state() != QProcess::NotRunning) {
        process.kill();
    }
}

void SearchWorker::onReadyRead()
{
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QString fullPath = line.trimmed();
        if (fullPath.isEmpty())
            continue;
        if (fullPath.contains("拒绝访问") || fullPath.contains("os error 5"))
            continue;
        QFileInfo info(fullPath);
        if (info.exists()) {
            emit resultFound(info.fileName(), info.absolutePath());
        }
    }
}
