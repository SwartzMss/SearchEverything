#ifndef SEARCHWORKER_H
#define SEARCHWORKER_H

#include <QObject>
#include <QProcess>

class SearchWorker : public QObject
{
    Q_OBJECT
public:
    explicit SearchWorker(QObject *parent = nullptr);

public slots:
    void start(const QString &rgExePath, const QStringList &arguments);
    void stop();

signals:
    void resultFound(const QString &name, const QString &path);
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

private slots:
    void onReadyRead();

private:
    QProcess process;
};

#endif // SEARCHWORKER_H
