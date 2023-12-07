#include "EmptyPreview.h"

#include "../../config/Config.h"

EmptyPreview::EmptyPreview(QWidget* parent)
        : AbstractInfoPreview({":/icon.png"}, VPKEDIT_FULL_TITLE, parent) {
    // We want the logo to be consistently greyed out
    this->setDisabled(true);
}
