#include "FileLoadErrorPreview.h"

FileLoadErrorPreview::FileLoadErrorPreview(QWidget* parent)
        : AbstractInfoPreview({":/error.png"}, tr("Failed to read file contents!\nPlease ensure that a game or another application is not using the file."), parent) {}
