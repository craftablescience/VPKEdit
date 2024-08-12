#pragma once

#include <cstddef>
#include <optional>
#include <string_view>
#include <tuple>
#include <vector>

#include <QDialog>

class QComboBox;
class QLineEdit;

class Window;

class VICEDialog : public QDialog {
	Q_OBJECT;

public:
	VICEDialog(Window* window_, QString path_, bool encrypt, QWidget* parent = nullptr);

	[[nodiscard]] std::optional<std::vector<std::byte>> getData();

	static std::optional<std::vector<std::byte>> encrypt(Window* window, const QString& path, QWidget* parent = nullptr);

	static std::optional<std::vector<std::byte>> decrypt(Window* window, const QString& path, QWidget* parent = nullptr);

private:
	Window* window;

	QComboBox* codes;
	QLineEdit* customCode;

	QString path;
	bool encrypting;

	static QList<std::pair<QString, std::string_view>> CODES;
};
