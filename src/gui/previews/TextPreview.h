#pragma once

#include <QTextEdit>

class TextPreview : public QTextEdit {
    Q_OBJECT;

public:
    static inline const QStringList EXTENSIONS {
        ".txt",
        ".md",
        ".gi",
        ".rc",
        ".res",
        ".vmt",
        ".vmf",
        ".vbsp",
        ".rad",
        ".nut",
        ".lua",
        ".gm",
        ".py",
        ".js",
        ".ts",
        ".cfg",
        ".kv",
        ".kv3",
        ".vdf",
        ".acf",
        ".ini",
        ".yml",
        ".yaml",
        ".toml",
        ".html",
        ".htm",
        ".xml",
        ".css",
        ".scss",
        ".sass",
        ".gitignore",
        "authors",
        "credits",
        "license",
        "readme",
    };

    explicit TextPreview(QWidget* parent = nullptr);
};
