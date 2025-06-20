#ifndef EXPORTWORKER_H
#define EXPORTWORKER_H

#include <QObject>
#include <QProcess>

class ExportWorker : public QObject
{
    Q_OBJECT
public:
    explicit ExportWorker(QObject *parent = nullptr);

public slots:
    void start(const QString &rgExePath, const QStringList &arguments, const QString &outputFile);

signals:
    void finished(bool success);

private slots:
    void onFinished(int exitCode, QProcess::ExitStatus status);

private:
    QProcess process;
    QString outFile;
};

#endif // EXPORTWORKER_H
