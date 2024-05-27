#pragma once

#include <QMessageBox>

namespace vpkedit {

class PackFile;

} // namespace vpkedit

class VerifySignatureDialog : public QMessageBox {
	Q_OBJECT;

public:
	explicit VerifySignatureDialog(vpkedit::PackFile& packFile, QWidget* parent = nullptr);

	static void showDialog(vpkedit::PackFile& packFile, QWidget* parent = nullptr);
};
