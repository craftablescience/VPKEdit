// ReSharper disable CppDFAMemoryLeak
// ReSharper disable CppUseStructuredBinding

#include "DMXPreview.h"

#include <algorithm>
#include <functional>
#include <span>

#include <kvpp/DMX.h>
#include <QTreeWidget>

using namespace kvpp;

void DMXPreview::initPlugin(IVPKEditWindowAccess_V3*) {}

void DMXPreview::initPreview(QWidget* parent) {
	this->preview = new QTreeWidget{parent};
	this->preview->setColumnCount(3);
	this->preview->setColumnWidth(0, 300);
	this->preview->setColumnWidth(1, 120);
	this->preview->setHeaderLabels({tr("Key"), tr("Value Type"), tr("Value")});
}

QWidget* DMXPreview::getPreview() const {
	return this->preview;
}

const QSet<QString>& DMXPreview::getPreviewExtensions() const {
	static const QSet<QString> EXTENSIONS{
		".dmx",
		".movieobjects",
		".sfm",
		".sfm_settings",
		".sfm_session",
		".sfm_trackgroup",
		".pcf",
		".gui",
		".schema",
		".preset",
		".facial_animation",
		".model",
		".ved",
		".mks",
		".vmks",
		".mp_preprocess",
		".mp_root",
		".mp_model",
		".mp_anim",
		".mp_physics",
		".mp_hitbox",
		".mp_materialgroup",
		".mp_keyvalues",
		".mp_eyes",
		".mp_bonemask",
		".tex",
		".world",
		".worldnode",
	};
	return EXTENSIONS;
}

QIcon DMXPreview::getIcon() const {
	// todo: cool icon
	return {};
}

int DMXPreview::setData(const QString&, const quint8* dataPtr, quint64 length) {
	this->preview->clear();

	std::unique_ptr<DMX> dmx;
	try {
		dmx = std::make_unique<DMX>(std::span{reinterpret_cast<const std::byte*>(dataPtr), length});
	} catch (const std::overflow_error&) {}
	if (!dmx || !*dmx) {
		emit this->showInfoPreview({":/icons/error.png"}, tr("Failed to parse DMX file."));
		return ERROR_SHOWED_OTHER_PREVIEW;
	}

	auto* root = new QTreeWidgetItem{this->preview};
	root->setText(0, QString{"%1 v%2"}.arg(QString{dmx->getFormatType().data()}.toUpper()).arg(dmx->getFormatVersion()));
	this->preview->addTopLevelItem(root);

	for (int i = 0; i < dmx->getPrefixAttributeContainerCount(); i++) {
		const auto& prefixContainer = dmx->getPrefixAttributeContainers()[i];

		auto* name = new QTreeWidgetItem{root};
		name->setText(0, tr("Prefix Attribute Container #%1").arg(i + 1));

		for (const auto& attribute : prefixContainer.getAttributes()) {
			auto* item = new QTreeWidgetItem{name};
			item->setText(0, attribute.getKey().data());
			item->setText(1, DMXValue::idToString(attribute.getValueType()).c_str());
			item->setText(2, attribute.getValueString().c_str());
		}
	}

	const auto& elements = dmx->getElements();
	QVector<std::array<std::byte, 16>> referencedElements;

	std::function<void(int, QTreeWidgetItem*, std::vector<int>&)> addElement;
	addElement = [&](int elementIndex, QTreeWidgetItem* parent, std::vector<int>& seenElements) {
		const auto& element = elements[elementIndex];

		if (parent == root && std::ranges::find(referencedElements, element.getUUID()) != referencedElements.end()) {
			return;
		}
		referencedElements.push_back(element.getUUID());

		auto* name = new QTreeWidgetItem{parent};
		name->setText(0, element.getKey().data());
		auto* type = new QTreeWidgetItem{name};
		type->setText(0, tr("Type"));
		type->setText(1, DMXValue::idToString(DMXValue::ID::STRING).c_str());
		type->setText(2, element.getType().data());
		auto* uuid = new QTreeWidgetItem{name};
		uuid->setText(0, tr("ID"));
		uuid->setText(1, DMXValue::idToString(DMXValue::ID::UUID).c_str());
		uuid->setText(2, QByteArray{reinterpret_cast<const char*>(element.getUUID().data()), static_cast<qsizetype>(element.getUUID().size())}.toHex());

		for (const auto& attribute : element.getAttributes()) {
			auto* item = new QTreeWidgetItem{name};
			item->setText(0, attribute.getKey().data());
			item->setText(1, DMXValue::idToString(attribute.getValueType()).c_str());
			item->setText(2, attribute.getValueString().c_str());

			if (std::ranges::find(seenElements, elementIndex) != seenElements.end()) {
				auto* dotdotdot = new QTreeWidgetItem{item};
				dotdotdot->setText(0, "...");
				continue;
			}

			if (attribute.getValueType() == DMXValue::ID::ELEMENT) {
				if (const auto elementValue = attribute.getValue<DMXValue::Element>(); elementValue.index != -2 && elementValue.index < elements.size()) {
					if (parent != root) {
						seenElements.push_back(elementIndex);
					}
					addElement(elementValue.index, item, seenElements);
					std::erase(seenElements, elementIndex);
				}
			} else if (attribute.getValueType() == DMXValue::ID::ARRAY_ELEMENT) {
				for (const auto& elementValue : attribute.getValue<std::vector<DMXValue::Element>>()) {
					if (elementValue.index != -2 && elementValue.index < elements.size()) {
						if (parent != root) {
							seenElements.push_back(elementIndex);
						}
						addElement(elementValue.index, item, seenElements);
						std::erase(seenElements, elementIndex);
					}
				}
			}
		}
	};

	std::vector<int> seen;
	for (int i = 0; i < elements.size(); i++) {
		addElement(i, root, seen);
	}

	root->setExpanded(true);
	for (int i = 0; i < root->childCount(); i++) {
		root->child(i)->setExpanded(true);
	}

	return ERROR_SHOWED_THIS_PREVIEW;
}
