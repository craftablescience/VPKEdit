// ReSharper disable CppDFAMemoryLeak

#include "VICEPreview.h"

#include <ranges>
#include <utility>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
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

void VICEPreview::initPlugin(IVPKEditWindowAccess_V3* windowAccess_) {
	this->windowAccess = windowAccess_;

	auto* opts = this->windowAccess->getOptions();

	if (!opts->contains(STR_VICE_CODE_INDEX)) {
		opts->setValue(STR_VICE_CODE_INDEX, 0);
	}

	if (!opts->contains(STR_VICE_CODE_VALUE)) {
		opts->setValue(STR_VICE_CODE_VALUE, QString{VICE::KnownCodes::DEFAULT.data()});
	}
}

void VICEPreview::initPreview(QWidget* parent) {
	this->preview = new QWidget{parent};
	this->preview->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	auto* layout = new QHBoxLayout{this->preview};

	auto* group = new QWidget{this->preview};
	group->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	layout->addWidget(group);
	auto* groupLayout = new QVBoxLayout{group};

	auto* label = new QLabel{tr("Decrypt file to view contents"), parent};
	label->setAlignment(Qt::AlignCenter);
	groupLayout->addWidget(label);

	auto* decryptButton = new QPushButton{tr("Decrypt"), parent};
	groupLayout->addWidget(decryptButton);
	QObject::connect(decryptButton, &QPushButton::clicked, this, [this] {
		//const auto data = VICEDialog::decrypt(this->selectedPath, this->windowAccess, this->preview);
		// todo: write data to pack file with remapped extension, remove existing file
		QMessageBox::information(this->preview, tr("Unimplemented"), tr("This function has not been reimplemented yet."));
	});
}

QWidget* VICEPreview::getPreview() const {
	return this->preview;
}

const QSet<QString>& VICEPreview::getPreviewExtensions() const {
	static const QSet<QString> EXTENSIONS{
		".ctx",
		".ekv",
		".nuc",
	};
	return EXTENSIONS;
}

QIcon VICEPreview::getIcon() const {
	// todo: cool icon
	return {};
}

IVPKEditPreviewPlugin_V1_3::Error VICEPreview::setData(const QString& path, const quint8*, quint64) {
	this->preview->setDisabled(this->windowAccess->isReadOnly());
	this->selectedPath = path;
	return ERROR_SHOWED_THIS_PREVIEW;
}
