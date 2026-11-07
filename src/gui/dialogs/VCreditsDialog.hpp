#pragma once

#include <QDialog>

class VCreditsDialog : public QDialog {
	Q_OBJECT;

public:
	explicit VCreditsDialog(QWidget* parent = nullptr);

	static void showCredits(QWidget* parent = nullptr);
};
