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
        ".vmf", // hey you never know
        ".vmt",
    };

    explicit TextPreview(QWidget* parent = nullptr);
};
