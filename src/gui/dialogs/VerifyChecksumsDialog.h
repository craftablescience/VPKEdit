#pragma once

#include <QMessageBox>

namespace vpkpp {

class PackFile;

} // namespace vpkpp

class VerifyChecksumsDialog : public QMessageBox {
	Q_OBJECT;

public:
	explicit VerifyChecksumsDialog(vpkpp::PackFile& packFile, QWidget* parent = nullptr);

	static void showDialog(vpkpp::PackFile& packFile, QWidget* parent = nullptr);
};
