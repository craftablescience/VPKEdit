#include "ErrorPreview.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>

ErrorPreview::ErrorPreview(QWidget* parent)
        : AbstractInfoPreview(
                {":/error.png"},
                tr("Failed to read file contents!\nPlease ensure that a game or another application is not using the VPK."),
                parent) {}
