# SearchEverything

一款基于 Qt 和 rg.exe 的 Windows 本地文件/内容极速搜索工具。

---

## ✨ 主要功能
- **多目录/全盘极速搜索**：支持文件名和内容搜索，速度极快
- **文件类型过滤**：支持通配符过滤（如 *.cpp;*.h;*.txt）
- **正则/普通字符串匹配**
- **结果导出**：一键导出搜索结果到 txt/csv
- **自动记忆上次搜索目录和rg.exe路径**
- **一键检查rg.exe版本**，推荐13.0及以上
- **支持大文件/大目录/多线程**
- **现代化简洁UI**

---

## 🚀 快速开始

1. **下载 [ripgrep (rg.exe)](https://github.com/BurntSushi/ripgrep/releases) 并放到本程序目录**
2. 运行 `SearchEverything.exe`
3. 选择 rg.exe 路径和要搜索的目录
4. 输入搜索内容，点击"搜索"
5. 可导出结果，或右键打开文件路径

---

## 🛠️ 构建与CI

本项目已集成 GitHub Actions 自动化：
- **build.yml**：每次 push/PR 自动编译并上传 release 产物
- **release.yml**：每次打 tag（如 v1.0.0）自动编译并发布 Release

> 详见 `.github/workflows/`

本地构建：
```bash
# 需安装 Qt 6.x + MSVC
qmake
nmake
```

---

## ⚙️ 依赖环境
- Qt 6.x (推荐 6.5+)
- MSVC 2019/2022
- ripgrep (rg.exe) 13.0 及以上

---

## 🤝 贡献
欢迎 issue、PR、建议！

---

## 📄 License
MIT
