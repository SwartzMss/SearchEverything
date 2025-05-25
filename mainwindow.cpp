#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>
#include <QTextStream>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , searchProcess(nullptr)
    , isSearching(false)
    , logFile(QCoreApplication::applicationDirPath() + "/SearchEverything.log")
    , logStream(&logFile)
    , exportProcess(nullptr)
{
    setupUI();
    setWindowTitle("SearchEverything");
    resize(800, 600);

    // 打开日志文件（追加模式）
    if (!logFile.open(QIODevice::Append | QIODevice::Text)) {
        QMessageBox::warning(this, "日志文件错误", "无法打开日志文件，日志功能不可用！");
    }

    // 加载配置
    loadConfig();

    updateButtonsState();
}

MainWindow::~MainWindow()
{
    if (searchProcess) {
        searchProcess->kill();
        delete searchProcess;
    }
    if (logFile.isOpen()) {
        logFile.close();
    }
}

void MainWindow::writeLog(const QString &msg)
{
    if (logFile.isOpen()) {
        logStream << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss ") << msg << "\n";
        logStream.flush();
    }
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // rg.exe 选择区域
    QHBoxLayout *rgLayout = new QHBoxLayout();
    rgPathEdit = new QLineEdit(this);
    rgPathEdit->setReadOnly(true);
    browseRgButton = new QPushButton("选择 rg.exe...", this);
    checkRgVersionButton = new QPushButton("检查rg.exe版本", this);
    rgLayout->addWidget(new QLabel("rg.exe 路径:", this));
    rgLayout->addWidget(rgPathEdit);
    rgLayout->addWidget(browseRgButton);
    rgLayout->addWidget(checkRgVersionButton);
    mainLayout->addLayout(rgLayout);

    // 路径选择区域
    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathEdit = new QLineEdit(this);
    pathEdit->setReadOnly(true);
    browseButton = new QPushButton("选择目录...", this);
    pathLayout->addWidget(new QLabel("搜索目录:", this));
    pathLayout->addWidget(pathEdit);
    pathLayout->addWidget(browseButton);
    mainLayout->addLayout(pathLayout);

    // 文件类型过滤（在搜索内容上面）
    QHBoxLayout *fileTypeLayout = new QHBoxLayout();
    fileTypeEdit = new QLineEdit(this);
    fileTypeEdit->setPlaceholderText("例如: *.cpp;*.h;*.txt");
    fileTypeLayout->addWidget(new QLabel("文件类型过滤:", this));
    fileTypeLayout->addWidget(fileTypeEdit);
    mainLayout->addLayout(fileTypeLayout);

    // 搜索输入区域
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchEdit = new QLineEdit(this);
    searchLayout->addWidget(new QLabel("搜索内容:", this));
    searchLayout->addWidget(searchEdit);
    mainLayout->addLayout(searchLayout);

    // 匹配模式单选按钮
    QHBoxLayout *matchModeLayout = new QHBoxLayout();
    fixedStringRadio = new QRadioButton("普通字符串匹配", this);
    regexRadio = new QRadioButton("正则表达式匹配", this);
    fixedStringRadio->setChecked(true);
    matchModeLayout->addWidget(fixedStringRadio);
    matchModeLayout->addWidget(regexRadio);
    mainLayout->addLayout(matchModeLayout);

    // rg.exe 命令显示区域
    cmdDisplayEdit = new QLineEdit(this);
    cmdDisplayEdit->setReadOnly(true);
    mainLayout->addWidget(new QLabel("rg.exe 命令:", this));
    mainLayout->addWidget(cmdDisplayEdit);

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    searchButton = new QPushButton("搜索", this);
    stopButton = new QPushButton("停止", this);
    exportButton = new QPushButton("导出...", this);
    stopButton->setEnabled(false);
    buttonLayout->addWidget(searchButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addWidget(exportButton);
    mainLayout->addLayout(buttonLayout);

    // 结果显示区域（表格）
    resultTable = new QTableWidget(this);
    resultTable->setColumnCount(2);
    QStringList headers;
    headers << "名称" << "路径";
    resultTable->setHorizontalHeaderLabels(headers);
    resultTable->horizontalHeader()->setStretchLastSection(true);
    resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultTable->setSelectionMode(QAbstractItemView::SingleSelection);
    resultTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mainLayout->addWidget(resultTable);

    // 状态栏
    statusBarWidget = new QStatusBar(this);
    setStatusBar(statusBarWidget);
    updateResultCount();

    // 右键菜单
    resultTableMenu = new QMenu(this);
    QAction *openPathAction = new QAction(QIcon::fromTheme("folder-open"), "打开路径", this);
    openPathAction->setIconVisibleInMenu(true);
    resultTableMenu->addAction(openPathAction);
    connect(resultTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::onResultTableContextMenuRequested);
    connect(openPathAction, &QAction::triggered, this, &MainWindow::onOpenPathAction);

    // 连接信号和槽
    connect(browseRgButton, &QPushButton::clicked, this, &MainWindow::onBrowseRgClicked);
    connect(browseButton, &QPushButton::clicked, this, &MainWindow::onBrowseDirClicked);
    connect(searchButton, &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::onExportClicked);
    connect(searchEdit, &QLineEdit::textChanged, this, &MainWindow::updateButtonsState);
    connect(fileTypeEdit, &QLineEdit::textChanged, this, &MainWindow::updateButtonsState);
    connect(fixedStringRadio, &QRadioButton::toggled, this, &MainWindow::updateButtonsState);
    connect(regexRadio, &QRadioButton::toggled, this, &MainWindow::updateButtonsState);
    connect(checkRgVersionButton, &QPushButton::clicked, this, &MainWindow::onCheckRgVersionClicked);
}

void MainWindow::loadConfig()
{
    QString configPath = QCoreApplication::applicationDirPath() + "/config.json";
    QFile configFile(configPath);
    
    if (configFile.open(QIODevice::ReadOnly)) {
        QByteArray data = configFile.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();
        
        // 加载rg.exe路径
        if (obj.contains("rg_exe_path")) {
            QString path = obj["rg_exe_path"].toString();
            if (QFileInfo::exists(path)) {
                rgExePath = path;
                rgPathEdit->setText(rgExePath);
            }
        }
        
        // 加载搜索目录
        if (obj.contains("search_directories") && obj["search_directories"].isArray()) {
            QJsonArray dirs = obj["search_directories"].toArray();
            if (!dirs.isEmpty()) {
                QString dir = dirs[0].toString(); // 使用第一个目录作为默认目录
                if (QDir(dir).exists()) {
                    currentPath = dir;
                    pathEdit->setText(currentPath);
                }
            }
        }
        
        configFile.close();
    }
}

void MainWindow::saveConfig()
{
    QString configPath = QCoreApplication::applicationDirPath() + "/config.json";
    QFile configFile(configPath);
    
    if (configFile.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        
        // 保存rg.exe路径
        obj["rg_exe_path"] = rgExePath;
        
        // 保存搜索目录
        QJsonArray dirs;
        dirs.append(currentPath);
        obj["search_directories"] = dirs;
        
        QJsonDocument doc(obj);
        configFile.write(doc.toJson());
        configFile.close();
    }
}

void MainWindow::onBrowseRgClicked()
{
    QString file = QFileDialog::getOpenFileName(this, "选择 rg.exe", QString(), "可执行文件 (*.exe)");
    if (!file.isEmpty()) {
        QFileInfo fileInfo(file);
        if (fileInfo.fileName().toLower() == "rg.exe") {
            rgExePath = file;
            rgPathEdit->setText(rgExePath);
            updateButtonsState();
            saveConfig(); // 保存配置
        } else {
            QMessageBox::warning(this, "警告", "请选择正确的 rg.exe 文件！");
        }
    }
}

void MainWindow::onBrowseDirClicked()
{
    if (!checkRgExe(true)) {
        return;
    }

    QString dir = QFileDialog::getExistingDirectory(this, "选择搜索目录");
    if (!dir.isEmpty()) {
        pathEdit->setText(dir);
        currentPath = dir;
        updateButtonsState();
        saveConfig(); // 保存配置
    }
}

void MainWindow::updateButtonsState()
{
    bool hasRgExe = checkRgExe(false);
    bool hasSearchDir = !currentPath.isEmpty();
    browseButton->setEnabled(hasRgExe);
    searchButton->setEnabled(hasRgExe && hasSearchDir && !isSearching);
    stopButton->setEnabled(isSearching);
    exportButton->setEnabled(hasRgExe && hasSearchDir);
}

bool MainWindow::checkRgExe(bool showWarning)
{
    if (rgExePath.isEmpty() || !QFileInfo::exists(rgExePath)) {
        if (showWarning) {
            QString msg = "请先选择 rg.exe 文件！";
            QMessageBox::warning(this, "警告", msg);
            writeLog("[警告] " + msg);
        }
        return false;
    }
    return true;
}

void MainWindow::onSearchClicked()
{
    if (!checkRgExe(true)) {
        return;
    }

    if (currentPath.isEmpty()) {
        QString msg = "请先选择搜索目录！";
        QMessageBox::warning(this, "警告", msg);
        writeLog("[警告] " + msg);
        return;
    }

    startSearch();
}

void MainWindow::startSearch()
{
    if (searchProcess) {
        delete searchProcess;
    }

    searchProcess = new QProcess(this);
    connect(searchProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onSearchOutput);
    connect(searchProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onSearchFinished);

    QStringList arguments;
    QString searchText = searchEdit->text();
    QString fileType = fileTypeEdit->text();

    if (searchText.isEmpty()) {
        arguments << "--files";
    } else {
        arguments << "-l";
        if (fixedStringRadio->isChecked()) {
            arguments << "-F";
        }
        arguments << searchText;
    }
    if (!fileType.isEmpty()) {
        QStringList patterns = fileType.split(';', Qt::SkipEmptyParts);
        for (const QString &pat : patterns) {
            arguments << "--glob=" + pat.trimmed();
        }
    }
    arguments << "--no-messages";
    arguments << "--glob=!System Volume Information/**";
    arguments << "--glob=!$RECYCLE.BIN/**";
    arguments << "--glob=!pagefile.sys";
    arguments << "--glob=!hiberfil.sys";
    arguments << "--glob=!swapfile.sys";
    arguments << currentPath;

    writeLog(QString("[startSearch] rgExePath: %1, arguments: %2").arg(rgExePath, arguments.join(" ")));
    cmdDisplayEdit->setText(rgExePath + " " + arguments.join(" "));
    writeLog(QString("[搜索] %1").arg(cmdDisplayEdit->text()));

    resultTable->setRowCount(0);
    searchProcess->setProcessChannelMode(QProcess::MergedChannels);
    searchProcess->start(rgExePath, arguments);
    
    isSearching = true;
    updateButtonsState();
    statusBarWidget->showMessage("搜索中，请等待...");
}

void MainWindow::onStopClicked()
{
    stopSearch();
}

void MainWindow::stopSearch()
{
    if (searchProcess && isSearching) {
        searchProcess->kill();
        isSearching = false;
        updateButtonsState();
        updateResultCount();
        writeLog("[stopSearch] 用户手动停止搜索。");
    }
}

void MainWindow::onSearchFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    writeLog(QString("[onSearchFinished] exitCode: %1, exitStatus: %2").arg(exitCode).arg(exitStatus));
    isSearching = false;
    updateButtonsState();
    if (exitStatus == QProcess::NormalExit) {
        if (exitCode == 0) {
            updateResultCount();
            writeLog("[搜索完成] 正常退出，找到匹配项。");
        } else if (exitCode == 1) {
            statusBarWidget->showMessage("未找到匹配项");
            writeLog("[搜索完成] 正常退出，未找到匹配项。");
        } else {
            statusBarWidget->showMessage("搜索出错，退出码：" + QString::number(exitCode));
            writeLog(QString("[搜索完成] 出错，rg.exe退出码: %1").arg(exitCode));
        }
    } else if (exitStatus == QProcess::CrashExit) {
        writeLog("[搜索进程崩溃] rg.exe crashed.");
        statusBarWidget->showMessage("rg.exe 进程崩溃");
    }
}

void MainWindow::onSearchOutput()
{
    writeLog("[onSearchOutput] called");
    if (searchProcess) {
        QString output = QString::fromUtf8(searchProcess->readAllStandardOutput());
        writeLog("[onSearchOutput] output: " + output);
        QStringList lines = output.split("\n", Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            QString fullPath = line.trimmed();
            if (!fullPath.isEmpty()) {
                if (fullPath.contains("拒绝访问") || fullPath.contains("os error 5")) {
                    // 过滤系统拒绝访问的输出，不写路径不存在日志
                    continue;
                }
                QFileInfo info(fullPath);
                if (info.exists()) {
                    int row = resultTable->rowCount();
                    resultTable->insertRow(row);
                    resultTable->setItem(row, 0, new QTableWidgetItem(info.fileName()));
                    resultTable->setItem(row, 1, new QTableWidgetItem(info.absolutePath()));
                } else {
                    writeLog("[onSearchOutput] 路径不存在: " + fullPath);
                }
            }
        }
        resultTable->sortItems(0, Qt::AscendingOrder);
        updateResultCount();
    }
}

void MainWindow::onExportClicked()
{
    if (!checkRgExe(true)) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        "导出搜索结果",
        QString(),
        "文本文件 (*.txt);;CSV文件 (*.csv)");

    if (!fileName.isEmpty()) {
        // 重新拼接rg.exe命令参数
        QStringList arguments;
        QString searchText = searchEdit->text();
        QString fileType = fileTypeEdit->text();
        if (searchText.isEmpty()) {
            arguments << "--files";
        } else {
            arguments << "-l";
            if (fixedStringRadio->isChecked()) {
                arguments << "-F";
            }
            arguments << searchText;
        }
        if (!fileType.isEmpty()) {
            QStringList patterns = fileType.split(';', Qt::SkipEmptyParts);
            for (const QString &pat : patterns) {
                arguments << "--glob=" + pat.trimmed();
            }
        }
        arguments << "--no-messages";
        arguments << "--glob=!System Volume Information/**";
        arguments << "--glob=!$RECYCLE.BIN/**";
        arguments << "--glob=!pagefile.sys";
        arguments << "--glob=!hiberfil.sys";
        arguments << "--glob=!swapfile.sys";
        arguments << currentPath;

        QProcess exportProcess;
        exportProcess.setProcessChannelMode(QProcess::MergedChannels);
        exportProcess.setStandardOutputFile(fileName, QIODevice::Truncate);
        writeLog(QString("[导出] rgExePath: %1, arguments: %2, output: %3").arg(rgExePath, arguments.join(" "), fileName));
        statusBarWidget->showMessage("正在导出，请等待...");
        exportProcess.start(rgExePath, arguments);
        bool ok = exportProcess.waitForFinished(-1);
        QString err = QString::fromUtf8(exportProcess.readAllStandardError());
        if (!err.isEmpty()) {
            writeLog("[导出stderr] " + err);
        }
        QFileInfo fi(fileName);
        if (ok && exportProcess.exitCode() == 0 && fi.exists() && fi.size() > 0) {
            QMessageBox::information(this, "成功", "搜索结果已成功导出！");
            statusBarWidget->showMessage("导出完成");
            writeLog("[导出完成] 正常退出，结果已保存。");
        } else {
            QMessageBox::critical(this, "错误", "rg.exe 执行导出时出错，或文件未生成/为空！");
            statusBarWidget->showMessage("导出失败");
            writeLog(QString("[导出失败] exitCode: %1, ok: %2, file: %3, size: %4").arg(exportProcess.exitCode()).arg(ok).arg(fileName).arg(fi.size()));
        }
    }
}

void MainWindow::addResultRow(const QString &fullPath)
{
    // 这个函数现在不再直接使用，解析逻辑移到onSearchOutput
    // 保留此函数框架，但实际逻辑需要根据解析结果调用setItem
    // 为了兼容之前的代码和避免删除，暂时保留，但实际填充表格逻辑在onSearchOutput
     QFileInfo info(fullPath);
     int row = resultTable->rowCount();
     resultTable->insertRow(row);
     resultTable->setItem(row, 0, new QTableWidgetItem(info.fileName())); // 名称列显示文件名
     resultTable->setItem(row, 1, new QTableWidgetItem(info.absolutePath())); // 路径列显示目录路径
     updateResultCount();
}

void MainWindow::updateResultCount()
{
    int count = resultTable->rowCount();
    statusBarWidget->showMessage(QString("共找到 %1 个结果").arg(count));
}

void MainWindow::onResultTableContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = resultTable->indexAt(pos);
    if (index.isValid()) {
        resultTableMenu->exec(resultTable->viewport()->mapToGlobal(pos));
    }
}

void MainWindow::onOpenPathAction()
{
    int row = resultTable->currentRow();
    if (row >= 0) {
        QString filePath = resultTable->item(row, 1)->text();
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    }
    updateResultCount();
}

void MainWindow::onCheckRgVersionClicked()
{
    if (rgExePath.isEmpty() || !QFileInfo::exists(rgExePath)) {
        QMessageBox::warning(this, "警告", "请先选择正确的 rg.exe 路径！");
        return;
    }
    QProcess proc;
    proc.start(rgExePath, QStringList() << "--version");
    if (!proc.waitForStarted(2000)) {
        QMessageBox::critical(this, "错误", "无法启动 rg.exe！");
        return;
    }
    if (!proc.waitForFinished(2000)) {
        QMessageBox::critical(this, "错误", "rg.exe 检查超时！");
        return;
    }
    QString version = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    if (version.isEmpty()) {
        version = QString::fromUtf8(proc.readAllStandardError()).trimmed();
    }
    if (version.isEmpty()) {
        QMessageBox::critical(this, "错误", "未能获取 rg.exe 版本信息！");
    } else {
        QMessageBox::information(this, "rg.exe 版本", version + "\n\n建议使用 13.0 及以上版本以获得最佳体验。");
    }
} 