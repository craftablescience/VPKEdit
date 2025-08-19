#include "VICEDialog.h"

#include <utility>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <vcryptpp/vcryptpp.h>

#include "../utility/Options.h"
#include "../Window.h"

using namespace vcryptpp;

QList<std::pair<QString, std::string_view>> VICEDialog::CODES{
	{"Default",                          VICE::KnownCodes::DEFAULT},
	{"Bloody Good Time",                 VICE::KnownCodes::BLOODY_GOOD_TIME},
	{"Contagion Weapons",                VICE::KnownCodes::CONTAGION_WEAPONS},
	{"Contagion Scripts",                VICE::KnownCodes::CONTAGION_SCRIPTS},
	{"Counter-Strike: Source",           VICE::KnownCodes::COUNTER_STRIKE_SOURCE},
	{"Counter-Strike: Global Offensive", VICE::KnownCodes::COUNTER_STRIKE_GLOBAL_OFFENSIVE},
	{"Counter-Strike: 2",                VICE::KnownCodes::COUNTER_STRIKE_2},
	{"Counter-Strike: ProMod",           VICE::KnownCodes::COUNTER_STRIKE_PROMOD},
	{"Day of Defeat: Source",            VICE::KnownCodes::DAY_OF_DEFEAT_SOURCE},
	{"Dystopia 1.2",                     VICE::KnownCodes::DYSTOPIA_1_2},
	{"Dystopia 1.3",                     VICE::KnownCodes::DYSTOPIA_1_3},
	{"Golden-Eye Source",                VICE::KnownCodes::GOLDEN_EYE_SOURCE},
	{"Half-Life 2: Capture The Flag",    VICE::KnownCodes::HALF_LIFE_2_CTF},
	{"Half-Life 2: Deathmatch",          VICE::KnownCodes::HALF_LIFE_2_DM},
	{"Insurgency",                       VICE::KnownCodes::INSURGENCY},
	{"Left 4 Dead 2",                    VICE::KnownCodes::LEFT_4_DEAD_2},
	{"No More Room In Hell",             VICE::KnownCodes::NO_MORE_ROOM_IN_HELL},
	{"Nuclear Dawn",                     VICE::KnownCodes::NUCLEAR_DAWN},
	{"Tactical Intervention",            VICE::KnownCodes::TACTICAL_INTERVENTION},
	{"Team Fortress 2",                  VICE::KnownCodes::TEAM_FORTRESS_2},
	{"Team Fortress 2 Items",            VICE::KnownCodes::TEAM_FORTRESS_2_ITEMS},
	{"The Ship",                         VICE::KnownCodes::THE_SHIP},
	{"Zombie Panic Source",              VICE::KnownCodes::ZOMBIE_PANIC_SOURCE},
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
	auto code = this->customCode->text();
	if (code.length() < 8) {
		code.resize(8);
	}
	if (this->encrypting) {
		return VICE::encrypt(*data, code.toLocal8Bit().constData());
	}
	return VICE::decrypt(*data, code.toLocal8Bit().constData());
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
