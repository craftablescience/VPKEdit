#include "VICEDialog.h"

#include <utility>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <vicepp/vicepp.h>

#include "../utility/Options.h"
#include "../Window.h"

QList<std::pair<QString, std::string_view>> VICEDialog::CODES{
	{"Default",                          vicepp::KnownCodes::DEFAULT},
	{"Contagion Weapons",                vicepp::KnownCodes::CONTAGION_WEAPONS},
	{"Contagion Scripts",                vicepp::KnownCodes::CONTAGION_SCRIPTS},
	{"Counter-Strike: Source",           vicepp::KnownCodes::COUNTER_STRIKE_SOURCE},
	{"Counter-Strike: Global Offensive", vicepp::KnownCodes::COUNTER_STRIKE_GLOBAL_OFFENSIVE},
	{"Counter-Strike: 2",                vicepp::KnownCodes::COUNTER_STRIKE_2},
	{"Counter-Strike: ProMod",           vicepp::KnownCodes::COUNTER_STRIKE_PROMOD},
	{"Day of Defeat: Source",            vicepp::KnownCodes::DAY_OF_DEFEAT_SOURCE},
	{"Dystopia 1.2",                     vicepp::KnownCodes::DYSTOPIA_1_2},
	{"Dystopia 1.3",                     vicepp::KnownCodes::DYSTOPIA_1_3},
	{"Golden-Eye Source",                vicepp::KnownCodes::GOLDEN_EYE_SOURCE},
	{"Half-Life 2: Capture The Flag",    vicepp::KnownCodes::HALF_LIFE_2_CTF},
	{"Half-Life 2: Deathmatch",          vicepp::KnownCodes::HALF_LIFE_2_DM},
	{"Insurgency",                       vicepp::KnownCodes::INSURGENCY},
	{"Left 4 Dead 2",                    vicepp::KnownCodes::LEFT_4_DEAD_2},
	{"No More Room In Hell",             vicepp::KnownCodes::NO_MORE_ROOM_IN_HELL},
	{"Nuclear Dawn",                     vicepp::KnownCodes::NUCLEAR_DAWN},
	{"Tactical Intervention",            vicepp::KnownCodes::TACTICAL_INTERVENTION},
	{"Team Fortress 2",                  vicepp::KnownCodes::TEAM_FORTRESS_2},
	{"Team Fortress 2 Items",            vicepp::KnownCodes::TEAM_FORTRESS_2_ITEMS},
	{"The Ship",                         vicepp::KnownCodes::THE_SHIP},
	{"Zombie Panic Source",              vicepp::KnownCodes::ZOMBIE_PANIC_SOURCE},
};

VICEDialog::VICEDialog(Window* window_, QString path_, bool encrypt, QWidget* parent)
		: QDialog(parent)
		, window(window_)
		, path(std::move(path_))
		, encrypting(encrypt) {
	this->setModal(true);
	this->setWindowTitle(this->encrypting ? tr("Encrypt File") : tr("Decrypt File"));

	auto* layout = new QFormLayout(this);

	auto* codesLabel = new QLabel(tr("Code:"), this);
	this->codes = new QComboBox(this);
	for (const auto& [codeName, code] : CODES) {
		this->codes->addItem(codeName);
	}
	this->codes->addItem("Custom");
	layout->addRow(codesLabel, this->codes);

	auto* customCodeLabel = new QLabel(tr("Value:"), this);
	this->customCode = new QLineEdit(this);
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
	this->codes->setCurrentIndex(Options::get<int>(STR_VICE_CODE_INDEX));
	this->customCode->setText(Options::get<QString>(STR_VICE_CODE_VALUE));

	QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, [this] {
		if (this->customCode->text().length() != 8) {
			return;
		}

		Options::set(STR_VICE_CODE_INDEX, this->codes->currentIndex());
		Options::set(STR_VICE_CODE_VALUE, this->customCode->text());
		this->accept();
	});
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &VICEDialog::reject);
}

std::optional<std::vector<std::byte>> VICEDialog::getData() {
	auto data = this->window->readBinaryEntry(this->path);
	if (!data) {
		return std::nullopt;
	}
	if (this->encrypting) {
		return vicepp::encrypt(*data, this->customCode->text().toLocal8Bit().constData());
	}
	return vicepp::decrypt(*data, this->customCode->text().toLocal8Bit().constData());
}

std::optional<std::vector<std::byte>> VICEDialog::encrypt(Window* window, const QString& path, QWidget* parent) {
	auto* dialog = new VICEDialog(window, path, true, parent);
	int ret = dialog->exec();
	dialog->deleteLater();
	if (ret != QDialog::Accepted) {
		return std::nullopt;
	}
	return dialog->getData();
}

std::optional<std::vector<std::byte>> VICEDialog::decrypt(Window* window, const QString& path, QWidget* parent) {
	auto* dialog = new VICEDialog(window, path, false, parent);
	int ret = dialog->exec();
	dialog->deleteLater();
	if (ret != QDialog::Accepted) {
		return std::nullopt;
	}
	return dialog->getData();
}
