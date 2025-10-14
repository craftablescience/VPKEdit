// ReSharper disable CppDFAMemoryLeak

#include "VICEPreview.h"

#include <QLabel>

void VICEPreview::initPlugin(IVPKEditWindowAccess_V2*) {}

void VICEPreview::initPreview(QWidget* parent) {
	this->preview = new QLabel{tr("Decrypt file to view contents"), parent};
	this->preview->setAlignment(Qt::AlignCenter);
	this->preview->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	this->preview->setDisabled(true);
}

QWidget* VICEPreview::getPreview() const {
	return this->preview;
}

QIcon VICEPreview::getIcon() const {
	// todo: cool icon
	return {};
}

IVPKEditPreviewPlugin_V1_1::Error VICEPreview::setData(const QString&, const quint8* dataPtr, quint64 length) {
	return ERROR_SHOWED_THIS_PREVIEW;
}
