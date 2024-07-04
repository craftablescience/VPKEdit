#pragma once

#include <QMessageBox>

namespace vpkpp {

class PackFile;

} // namespace vpkpp

class VerifySignatureDialog : public QMessageBox {
	Q_OBJECT;

public:
	explicit VerifySignatureDialog(vpkpp::PackFile& packFile, QWidget* parent = nullptr);

	static void showDialog(vpkpp::PackFile& packFile, QWidget* parent = nullptr);
};
