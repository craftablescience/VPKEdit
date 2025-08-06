#pragma once

#include <QDialog>

class CreditsDialog : public QDialog {
	Q_OBJECT;

public:
	explicit CreditsDialog(QWidget* parent = nullptr);

	static void showDialog(QWidget* parent = nullptr);
};
