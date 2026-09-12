#include "VNavigationBar.hpp"

#include <QActionGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QStackedLayout>
#include <QTimer>
#include <QToolButton>

#include "utility/VSettings.hpp"

VNavigationBar::VNavigationBar(QWidget* parent) : QWidget{parent} {
	// --- Widgets ---

	auto* layout = new QHBoxLayout{this};
	layout->setContentsMargins(0, 0, 0, 0);

	// ------ View mode

	auto* dirViewModeIcons = new QAction{this->style()->standardIcon(QStyle::SP_FileDialogContentsView), tr("&Icons"), this};
	dirViewModeIcons->setShortcut(Qt::CTRL | Qt::Key_1);
	auto* dirViewModeCompact = new QAction{this->style()->standardIcon(QStyle::SP_FileDialogDetailedView), tr("&Compact"), this};
	dirViewModeCompact->setShortcut(Qt::CTRL | Qt::Key_2);
	auto* dirViewModeDetails = new QAction{this->style()->standardIcon(QStyle::SP_FileDialogInfoView), tr("&Details"), this};
	dirViewModeDetails->setShortcut(Qt::CTRL | Qt::Key_3);

	auto* dirViewModeButton = new QToolButton{this};
	dirViewModeButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
	dirViewModeButton->setDefaultAction(dirViewModeIcons);

	auto* dirViewModeGroup = new QActionGroup{this};
	auto* dirViewModeMenu = new QMenu{dirViewModeButton};
	for (auto* action : {dirViewModeIcons, dirViewModeCompact, dirViewModeDetails}) {
		action->setCheckable(true);
		dirViewModeGroup->addAction(action);
		dirViewModeMenu->addAction(action);
	}
	dirViewModeIcons->setChecked(true);

	dirViewModeButton->setMenu(dirViewModeMenu);
	dirViewModeButton->setPopupMode(QToolButton::MenuButtonPopup);

	layout->addWidget(dirViewModeButton);

	// ------ Navbar

	this->navigationBarLayout = new QStackedLayout;
	this->navigationBarLayout->setStackingMode(QStackedLayout::StackingMode::StackAll);

	this->pathEdit = new QLineEdit{this};
	this->navigationBarLayout->addWidget(this->pathEdit);

	auto* componentsParent = new QWidget{this};
	this->componentLayout = new QHBoxLayout{componentsParent};
	this->componentLayout->setContentsMargins(0, 0, 0, 0);
	this->componentLayout->addStretch();
	this->navigationBarLayout->addWidget(componentsParent);

	layout->addLayout(this->navigationBarLayout);

	// --- Logic ---

	connect(dirViewModeIcons, &QAction::triggered, dirViewModeButton, [this, dirViewModeIcons, dirViewModeButton] {
		dirViewModeButton->setDefaultAction(dirViewModeIcons);
		VSettings::set(VSettings::INT_DIR_VIEW_MODE, 0);
		emit this->dirViewModeUpdated(VPreviewDirectoryParent::ViewMode::ICONS);
	});
	connect(dirViewModeCompact, &QAction::triggered, dirViewModeButton, [this, dirViewModeCompact, dirViewModeButton] {
		dirViewModeButton->setDefaultAction(dirViewModeCompact);
		VSettings::set(VSettings::INT_DIR_VIEW_MODE, 1);
		emit this->dirViewModeUpdated(VPreviewDirectoryParent::ViewMode::COMPACT);
	});
	connect(dirViewModeDetails, &QAction::triggered, dirViewModeButton, [this, dirViewModeDetails, dirViewModeButton] {
		dirViewModeButton->setDefaultAction(dirViewModeDetails);
		VSettings::set(VSettings::INT_DIR_VIEW_MODE, 2);
		emit this->dirViewModeUpdated(VPreviewDirectoryParent::ViewMode::DETAILS);
	});
	QTimer::singleShot(0, this, [dirViewModeIcons, dirViewModeCompact, dirViewModeDetails] {
		switch (VSettings::get<int>(VSettings::INT_DIR_VIEW_MODE)) {
			default:
			case 0: dirViewModeIcons->trigger();   break;
			case 1: dirViewModeCompact->trigger(); break;
			case 2: dirViewModeDetails->trigger(); break;
		}
	});

	// todo: when clicking outside the buttons, allow typing into the currently "placeholder" LineEdit

	// --- Finalize ---

	this->clearPreview();
}

void VNavigationBar::setPath(const QString& path) {
	qDeleteAll(this->components);
	this->components.clear();

	if (path.isEmpty()) {
		this->clearPreview();
		return;
	}

	const auto addButton = [this](const QString& name) {
		auto* button = new QPushButton{name, this};
		button->setFlat(true);
		button->setFixedWidth(8 + button->fontMetrics().horizontalAdvance(name) + 8);
		this->components.append(button);
		this->componentLayout->insertWidget(this->componentLayout->count() - 1, button);

		connect(button, &QPushButton::clicked, this, [this, button] {
			QString out;
			for (const auto* widget : this->components) {
				if (const auto* b = qobject_cast<const QPushButton*>(widget)) {
					out += b->text();
				}
				if (const auto* l = qobject_cast<const QLabel*>(widget)) {
					out += l->text();
				}
				if (widget == button) {
					break;
				}
			}
			emit this->pathModified(out);
		});
	};

	const auto parts = path.split('/');
	for (int i = 0; i < parts.size() - 1; i++) {
		addButton(parts[i]);

		auto* label = new QLabel{"/", this};
		this->components.append(label);
		this->componentLayout->insertWidget(this->componentLayout->count() - 1, label);
	}
	if (!parts.isEmpty()) {
		addButton(parts.last());
	}

	this->pathEdit->setPlaceholderText({});

	this->navigationBarLayout->setCurrentIndex(1);
}

void VNavigationBar::clearPreview() {
	this->pathEdit->setPlaceholderText(tr("Navigate..."));

	this->navigationBarLayout->setCurrentIndex(0);

	qDeleteAll(this->components);
	this->components.clear();
}

void VNavigationBar::onDirSelected(const QString& path, vpkpp::Attribute, const QStringList&, const QMap<QString, std::optional<vpkpp::Entry>>&) {
	this->setPath(path);
}

void VNavigationBar::onFileSelected(const QString& path, const QByteArray&) {
	this->setPath(path);
}
