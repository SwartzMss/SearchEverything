#include "exportworker.h"
#include <QFileInfo>

ExportWorker::ExportWorker(QObject *parent) : QObject(parent)
{
    connect(&process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &ExportWorker::onFinished);
}

void ExportWorker::start(const QString &rgExePath, const QStringList &arguments, const QString &outputFile)
{
    outFile = outputFile;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setStandardOutputFile(outputFile, QIODevice::Truncate);
    process.start(rgExePath, arguments);
}

void ExportWorker::onFinished(int exitCode, QProcess::ExitStatus status)
{
    QFileInfo fi(outFile);
    bool success = (status == QProcess::NormalExit && exitCode == 0 && fi.exists() && fi.size() > 0);
    emit finished(success);
}
