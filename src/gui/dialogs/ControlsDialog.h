#pragma once

#include <QDialog>

class ControlsDialog : public QDialog {
	Q_OBJECT;

public:
	explicit ControlsDialog(QWidget* parent = nullptr);

	static void showControlsDialog(QWidget* parent = nullptr);
};
