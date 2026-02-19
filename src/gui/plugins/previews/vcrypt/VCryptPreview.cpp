// ReSharper disable CppDFAMemoryLeak
// ReSharper disable CppRedundantQualifier

#include "VCryptPreview.h"

#include <ranges>
#include <utility>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>
#include <vcryptpp/vcryptpp.h>

using namespace vcryptpp;

QList<std::pair<QString, std::string_view>> VICEDialog::CODES{
	{"Default",                                VICE::KnownCodes::DEFAULT},
	{"Default (Level Configs)",                VICE::KnownCodes::GPU_DEFAULT},
	{"Alien Swarm (Level Configs)",            VICE::KnownCodes::GPU_ALIEN_SWARM},
	{"Bloody Good Time",                       VICE::KnownCodes::BLOODY_GOOD_TIME},
	{"Contagion (Weapons)",                    VICE::KnownCodes::CONTAGION_WEAPONS},
	{"Contagion (Scripts)",                    VICE::KnownCodes::CONTAGION_SCRIPTS},
	{"Counter-Strike: Source",                 VICE::KnownCodes::COUNTER_STRIKE_SOURCE},
	{"Counter-Strike: Global Offensive",       VICE::KnownCodes::COUNTER_STRIKE_GLOBAL_OFFENSIVE},
	{"Counter-Strike: 2",                      VICE::KnownCodes::COUNTER_STRIKE_2},
	{"Counter-Strike: ProMod",                 VICE::KnownCodes::COUNTER_STRIKE_PROMOD},
	{"Day of Defeat: Source",                  VICE::KnownCodes::DAY_OF_DEFEAT_SOURCE},
	{"DotA 2 (Level Configs)",                 VICE::KnownCodes::GPU_DOTA_2},
	{"Dystopia (1.2)",                         VICE::KnownCodes::DYSTOPIA_1_2},
	{"Dystopia (1.3)",                         VICE::KnownCodes::DYSTOPIA_1_3},
	{"Fairy Tale Busters",                     VICE::KnownCodes::FAIRY_TALE_BUSTERS},
	{"Fortress Forever (Pre-Greenlight)",      VICE::KnownCodes::FORTRESS_FOREVER_PRE_GREENLIGHT},
	{"Golden-Eye Source",                      VICE::KnownCodes::GOLDEN_EYE_SOURCE},
	{"Half-Life 2: Capture The Flag",          VICE::KnownCodes::HALF_LIFE_2_CTF},
	{"Half-Life 2: Deathmatch",                VICE::KnownCodes::HALF_LIFE_2_DM},
	{"Half-Life 2: Episode 2 (Level Configs)", VICE::KnownCodes::GPU_HALF_LIFE_2_EPISODE_2},
	{"Insurgency",                             VICE::KnownCodes::INSURGENCY},
	{"Left 4 Dead 1 (Level Configs)",          VICE::KnownCodes::GPU_LEFT_4_DEAD_1},
	{"Left 4 Dead 2",                          VICE::KnownCodes::LEFT_4_DEAD_2},
	{"Left 4 Dead 2 (Level Configs)",          VICE::KnownCodes::GPU_LEFT_4_DEAD_2},
	{"No More Room In Hell",                   VICE::KnownCodes::NO_MORE_ROOM_IN_HELL},
	{"Nuclear Dawn",                           VICE::KnownCodes::NUCLEAR_DAWN},
	{"Portal 2 (Level Configs)",               VICE::KnownCodes::GPU_PORTAL_2},
	{"Tactical Intervention",                  VICE::KnownCodes::TACTICAL_INTERVENTION},
	{"Team Fortress 2",                        VICE::KnownCodes::TEAM_FORTRESS_2},
	{"Team Fortress 2 (Items)",                VICE::KnownCodes::TEAM_FORTRESS_2_ITEMS},
	{"Team Fortress 2 (Level Configs)",        VICE::KnownCodes::GPU_TEAM_FORTRESS_2},
	{"The Ship",                               VICE::KnownCodes::THE_SHIP},
	{"Valve Tracker",                          VICE::KnownCodes::VALVE_TRACKER},
	{"Zombie Panic Source",                    VICE::KnownCodes::ZOMBIE_PANIC_SOURCE},
};

VICEDialog::VICEDialog(QString path_, bool encrypt, IVPKEditWindowAccess_V3* windowAccess_, QWidget* parent)
		: QDialog(parent)
		, path(std::move(path_))
		, encrypting(encrypt)
		, windowAccess(windowAccess_) {
	this->setModal(true);
	this->setWindowTitle(this->encrypting ? tr("Encrypt File") : tr("Decrypt File"));

	auto* layout = new QFormLayout(this);

	auto* codesLabel = new QLabel(tr("Code:"), this);
	this->codes = new QComboBox(this);
	for (const auto& codeName: CODES | std::views::keys) {
		this->codes->addItem(codeName);
	}
	this->codes->addItem("Custom");
	layout->addRow(codesLabel, this->codes);

	auto* customCodeLabel = new QLabel(tr("Value:"), this);
	this->customCode = new QLineEdit(this);
	this->customCode->setMaxLength(8);
	layout->addRow(customCodeLabel, this->customCode);

	auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	layout->addWidget(buttonBox);

	QObject::connect(this->codes, &QComboBox::currentIndexChanged, [this](int index) {
		if (index == CODES.size()) {
			this->customCode->setDisabled(false);
		} else {
			this->customCode->setDisabled(true);
			this->customCode->setText(CODES[index].second.data());
		}
	});
	this->codes->setCurrentIndex(this->windowAccess->getOptions()->value(STR_VICE_CODE_INDEX, 0).toInt());
	this->customCode->setText(this->windowAccess->getOptions()->value(STR_VICE_CODE_VALUE, QString{}).toString());

	QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, [this] {
		if (this->customCode->text().length() != 8) {
			return;
		}

		this->windowAccess->getOptions()->setValue(STR_VICE_CODE_INDEX, this->codes->currentIndex());
		this->windowAccess->getOptions()->setValue(STR_VICE_CODE_VALUE, this->customCode->text());
		this->accept();
	});
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &VICEDialog::reject);
}

std::optional<std::vector<std::byte>> VICEDialog::getData() const {
	QByteArray data;
	if (!this->windowAccess->readBinaryEntry(this->path, data)) {
		return std::nullopt;
	}
	auto code = this->customCode->text();
	if (code.length() < 8) {
		code.resize(8);
	}
	if (this->encrypting) {
		return VICE::encrypt({reinterpret_cast<const std::byte*>(data.data()), static_cast<std::span<const std::byte>::size_type>(data.size())}, code.toLocal8Bit().constData());
	}
	return VICE::decrypt({reinterpret_cast<const std::byte*>(data.data()), static_cast<std::span<const std::byte>::size_type>(data.size())}, code.toLocal8Bit().constData());
}

std::optional<std::vector<std::byte>> VICEDialog::encrypt(const QString& path, IVPKEditWindowAccess_V3* windowAccess, QWidget* parent) {
	auto* dialog = new VICEDialog(path, true, windowAccess, parent);
	const int ret = dialog->exec();
	dialog->deleteLater();
	if (ret != QDialog::Accepted) {
		return std::nullopt;
	}
	return dialog->getData();
}

std::optional<std::vector<std::byte>> VICEDialog::decrypt(const QString& path, IVPKEditWindowAccess_V3* windowAccess, QWidget* parent) {
	auto* dialog = new VICEDialog(path, false, windowAccess, parent);
	const int ret = dialog->exec();
	dialog->deleteLater();
	if (ret != QDialog::Accepted) {
		return std::nullopt;
	}
	return dialog->getData();
}

void VCryptPreview::initPlugin(IVPKEditWindowAccess_V3* windowAccess_) {
	this->windowAccess = windowAccess_;

	auto* opts = this->windowAccess->getOptions();

	if (!opts->contains(STR_VICE_CODE_INDEX)) {
		opts->setValue(STR_VICE_CODE_INDEX, 0);
	}

	if (!opts->contains(STR_VICE_CODE_VALUE)) {
		opts->setValue(STR_VICE_CODE_VALUE, QString{VICE::KnownCodes::DEFAULT.data()});
	}
}

void VCryptPreview::initPreview(QWidget* parent) {
	this->preview = new QWidget{parent};
	this->preview->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	auto* layout = new QHBoxLayout{this->preview};

	auto* label = new QLabel{tr("Decrypt file to view contents"), parent};
	label->setAlignment(Qt::AlignCenter);
	label->setDisabled(true);
	layout->addWidget(label);
}

QWidget* VCryptPreview::getPreview() const {
	return this->preview;
}

const QSet<QString>& VCryptPreview::getPreviewExtensions() const {
	static const QSet<QString> EXTENSIONS{
		".ctx",
		".ekv",
		".nuc",
		".vfont",
	};
	return EXTENSIONS;
}

QIcon VCryptPreview::getIcon() const {
	// todo: cool icon
	return {};
}

int VCryptPreview::setData(const QString& path, const quint8*, quint64) {
	return ERROR_SHOWED_THIS_PREVIEW;
}

void VCryptPreview::initContextMenu(int contextMenuType, QMenu* contextMenu) {
	if (contextMenuType != CONTEXT_MENU_TYPE_FILE) {
		return;
	}

	this->encryptionMenus.push_back(contextMenu->addMenu(this->getIcon(), "VCrypt"));
	this->encryptICEActions.push_back(this->encryptionMenus.back()->addAction(this->getIcon(), tr("Encrypt ICE"), [this] {
		for (const auto& path : this->selectedPaths) {
			if (path.endsWith(".txt") || path.endsWith(".kv") || path.endsWith(".nut")) {
				this->encryptICE(path);
			}
		}
	}));
	this->decryptICEActions.push_back(this->encryptionMenus.back()->addAction(this->getIcon(), tr("Decrypt ICE"), [this] {
		for (const auto& path : this->selectedPaths) {
			if (path.endsWith(".ctx") || path.endsWith(".ekv") || path.endsWith(".nuc")) {
				this->decryptICE(path);
			}
		}
	}));
	this->encryptFontActions.push_back(this->encryptionMenus.back()->addAction(this->getIcon(), tr("Encrypt Font"), [this] {
		for (const auto& path : this->selectedPaths) {
			if (path.endsWith(".ttf")) {
				this->encryptFont(path);
			}
		}
	}));
	this->decryptFontActions.push_back(this->encryptionMenus.back()->addAction(this->getIcon(), tr("Decrypt Font"), [this] {
		for (const auto& path : this->selectedPaths) {
			if (path.endsWith(".vfont")) {
				this->decryptFont(path);
			}
		}
	}));
}

void VCryptPreview::updateContextMenu(int contextMenuType, const QStringList& paths) {
	if (contextMenuType != CONTEXT_MENU_TYPE_FILE) {
		return;
	}
	this->selectedPaths = paths;

	bool encryptICEAction = false;
	bool decryptICEAction = false;
	bool encryptFontAction = false;
	bool decryptFontAction = false;

	for (const auto& path : paths) {
		if (path.endsWith(".txt") || path.endsWith(".kv") || path.endsWith(".nut")) {
			encryptICEAction = true;
		} else if (path.endsWith(".ctx") || path.endsWith(".ekv") || path.endsWith(".nuc")) {
			decryptICEAction = true;
		} else if (path.endsWith(".ttf")) {
			encryptFontAction = true;
		} else if (path.endsWith(".vfont")) {
			decryptFontAction = true;
		}
	}

	for (auto* action : this->encryptICEActions) { action->setVisible(encryptICEAction); }
	for (auto* action : this->decryptICEActions) { action->setVisible(decryptICEAction); }
	for (auto* action : this->encryptFontActions) { action->setVisible(encryptFontAction); }
	for (auto* action : this->decryptFontActions) { action->setVisible(decryptFontAction); }

	for (const auto* menu : this->encryptionMenus) { menu->menuAction()->setVisible(encryptICEAction || decryptICEAction || encryptFontAction || decryptFontAction); }
}

void VCryptPreview::encryptICE(const QString& path) const {
	if (const auto data = VICEDialog::encrypt(path, this->windowAccess, this->preview)) {
		QString newPath;
		if (path.endsWith(".txt")) {
			newPath = path.sliced(0, path.size() - 4) + ".ctx";
		} else if (path.endsWith(".kv")) {
			newPath = path.sliced(0, path.size() - 4) + ".ekv";
		} else if (path.endsWith(".nut")) {
			newPath = path.sliced(0, path.size() - 4) + ".nuc";
		}
		this->windowAccess->renameFile(path, newPath);
		this->windowAccess->editFileContents(newPath, {reinterpret_cast<const char*>(data->data()), static_cast<qsizetype>(data->size())});
	}
}

void VCryptPreview::decryptICE(const QString& path) const {
	if (const auto data = VICEDialog::decrypt(path, this->windowAccess, this->preview)) {
		QString newPath;
		if (path.endsWith(".ctx")) {
			newPath = path.sliced(0, path.size() - 4) + ".txt";
		} else if (path.endsWith(".ekv")) {
			newPath = path.sliced(0, path.size() - 4) + ".kv";
		} else if (path.endsWith(".nuc")) {
			newPath = path.sliced(0, path.size() - 4) + ".nut";
		}
		this->windowAccess->renameFile(path, newPath);
		this->windowAccess->editFileContents(newPath, {reinterpret_cast<const char*>(data->data()), static_cast<qsizetype>(data->size())});
	}
}

void VCryptPreview::encryptFont(const QString& path) const {
	QByteArray decryptedData;
	if (!this->windowAccess->readBinaryEntry(path, decryptedData)) {
		return;
	}
	const auto data = VFONT::encrypt({reinterpret_cast<const std::byte*>(decryptedData.data()), static_cast<std::span<const std::byte>::size_type>(decryptedData.size())});
	if (!data.empty()) {
		const QString newPath = path.sliced(0, path.size() - 4) + ".vfont";
		this->windowAccess->renameFile(path, newPath);
		this->windowAccess->editFileContents(newPath, {reinterpret_cast<const char*>(data.data()), static_cast<qsizetype>(data.size())});
	}
}

void VCryptPreview::decryptFont(const QString& path) const {
	QByteArray encryptedData;
	if (!this->windowAccess->readBinaryEntry(path, encryptedData)) {
		return;
	}
	const auto data = VFONT::decrypt({reinterpret_cast<const std::byte*>(encryptedData.data()), static_cast<std::span<const std::byte>::size_type>(encryptedData.size())});
	if (!data.empty()) {
		const QString newPath = path.sliced(0, path.size() - 6) + ".ttf";
		this->windowAccess->renameFile(path, newPath);
		this->windowAccess->editFileContents(newPath, {reinterpret_cast<const char*>(data.data()), static_cast<qsizetype>(data.size())});
	}
}
