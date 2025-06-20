#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QRadioButton>
#include <QProcess>
#include <QThread>
#include "searchworker.h"
#include "exportworker.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QTableWidget>
#include <QStatusBar>
#include <QMenu>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onBrowseRgClicked();
    void onBrowseDirClicked();
    void onSearchClicked();
    void onStopClicked();
    void onExportClicked();
    void onExportFinished(bool success);
    void onSearchFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onSearchResult(const QString &name, const QString &path);
    void onResultTableContextMenuRequested(const QPoint &pos);
    void onOpenPathAction();
    void onCheckRgVersionClicked();

private:
    void setupUI();
    void startSearch();
    void stopSearch();
    void updateButtonsState();
    bool checkRgExe(bool showWarning = true);
    void writeLog(const QString &msg);
    void addResultRow(const QString &fullPath);
    void updateResultCount();
    void loadConfig();
    void saveConfig();

    QLineEdit *rgPathEdit;
    QLineEdit *pathEdit;
    QLineEdit *searchEdit;
    QLineEdit *fileTypeEdit;
    QRadioButton *fixedStringRadio;
    QRadioButton *regexRadio;
    QPushButton *browseRgButton;
    QPushButton *browseButton;
    QPushButton *searchButton;
    QPushButton *stopButton;
    QPushButton *exportButton;
    QPushButton *checkRgVersionButton;
    QTableWidget *resultTable;
    QStatusBar *statusBarWidget;
    QMenu *resultTableMenu;
    QLineEdit *cmdDisplayEdit;

    QThread *searchThread = nullptr;
    SearchWorker *searchWorker = nullptr;
    QThread *exportThread = nullptr;
    ExportWorker *exportWorker = nullptr;
    QString currentPath;
    QString rgExePath;
    bool isSearching;

    QFile logFile;
    QTextStream logStream;
};

#endif // MAINWINDOW_H 