#pragma once

#include <QMessageBox>

namespace vpkedit {

class PackFile;

} // namespace vpkedit

class VerifyChecksumsDialog : public QMessageBox {
	Q_OBJECT;

public:
	explicit VerifyChecksumsDialog(vpkedit::PackFile& packFile, QWidget* parent = nullptr);

	static void showDialog(vpkedit::PackFile& packFile, QWidget* parent = nullptr);
};
