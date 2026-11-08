// ReSharper disable CppDFAMemoryLeak
// ReSharper disable CppUseStructuredBinding

#include "KeyValuesPreview.hpp"

#include <algorithm>
#include <functional>
#include <span>

#include <kvpp/DMX.h>
#include <QTreeWidget>

void KeyValuesPreview::initPlugin(VWindowAccess_V6_0*) {}

// todo: support KV1 binary

const QSet<QString>& KeyValuesPreview::getFileExtensions() const {
	static const QSet<QString> EXTENSIONS{
		"dmx",
		"facial_animation",
		"gui",
		"mks",
		"model",
		"movieobjects",
		"mp_anim",
		"mp_bonemask",
		"mp_eyes",
		"mp_hitbox",
		"mp_keyvalues",
		"mp_materialgroup",
		"mp_model",
		"mp_physics",
		"mp_preprocess",
		"mp_root",
		"pcf",
		"preset",
		"schema",
		"sfm",
		"sfm_session",
		"sfm_settings",
		"sfm_trackgroup",
		"tex",
		"ved",
		"vmks",
		"world",
		"worldnode",
	};
	return EXTENSIONS;
}

int KeyValuesPreview::getPriority() const {
	return 50;
}

QWidget* KeyValuesPreview::getPreview(QWidget* parent) const {
	auto* preview = new QTreeWidget{parent};
	preview->setColumnCount(3);
	preview->setColumnWidth(0, 300);
	preview->setColumnWidth(1, 120);
	preview->setHeaderLabels({tr("Key"), tr("Value Type"), tr("Value")});
	return preview;
}

QString KeyValuesPreview::setData(QWidget* preview_, const QString&, const quint8* dataPtr, quint64 dataLen) {
	auto* preview = qobject_cast<QTreeWidget*>(preview_);
	if (!preview) {
		return UNEXPECTED_ERROR_MSG.toString();
	}

	preview->clear();

	std::unique_ptr<kvpp::DMX> dmx;
	try {
		dmx = std::make_unique<kvpp::DMX>(std::span{reinterpret_cast<const std::byte*>(dataPtr), dataLen});
	} catch (const std::overflow_error&) {}
	if (!dmx || !*dmx) {
		return tr("Failed to parse DMX file.");
	}

	auto* root = new QTreeWidgetItem{preview};
	QString encodingType;
	bool encodingTypeOld = false;
	switch (dmx->getEncodingType()) {
		case kvpp::DMX::ENCODING_INVALID:             break;
		case kvpp::DMX::ENCODING_BINARY_OLD:          encodingType = "binary";           encodingTypeOld = true; break;
		case kvpp::DMX::ENCODING_BINARY_OLD_SFM:      encodingType = "binary_sfm";       encodingTypeOld = true; break;
		case kvpp::DMX::ENCODING_BINARY:              encodingType = "binary";           break;
		case kvpp::DMX::ENCODING_BINARY_SEQIDS:       encodingType = "binary_seqids";    break;
		case kvpp::DMX::ENCODING_KEYVALUES2_OLD:      encodingType = "keyvalues2";       encodingTypeOld = true; break;
		case kvpp::DMX::ENCODING_KEYVALUES2_FLAT_OLD: encodingType = "keyvalues2_flat";  encodingTypeOld = true; break;
		case kvpp::DMX::ENCODING_KEYVALUES2:          encodingType = "keyvalues2";       break;
		case kvpp::DMX::ENCODING_KEYVALUES2_FLAT:     encodingType = "keyvalues2_flat";  break;
		case kvpp::DMX::ENCODING_KEYVALUES2_NOIDS:    encodingType = "keyvalues2_noids"; break;
	}
	root->setText(0, QString{"%1 \"%2\"%3 v%4, %5 %6 v%7"}
		.arg(tr("Encoding"), encodingType, encodingTypeOld ? QString{" (%1)"}.arg(tr("Old")) : "")
		.arg(dmx->getEncodingVersion())
		.arg(tr("Format"), QString{dmx->getFormatType().data()}.toUpper())
		.arg(dmx->getFormatVersion()));
	preview->addTopLevelItem(root);

	for (int i = 0; i < dmx->getPrefixAttributeContainerCount(); i++) {
		const auto& prefixContainer = dmx->getPrefixAttributeContainers()[i];

		auto* name = new QTreeWidgetItem{root};
		name->setText(0, tr("Prefix Attribute Container #%1").arg(i + 1));

		for (const auto& attribute : prefixContainer.getAttributes()) {
			auto* item = new QTreeWidgetItem{name};
			item->setText(0, attribute.getKey().data());
			item->setText(1, kvpp::DMXValue::idToString(attribute.getValueType()).c_str());
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
		type->setText(1, kvpp::DMXValue::idToString(kvpp::DMXValue::ID::STRING).c_str());
		type->setText(2, element.getType().data());
		auto* uuid = new QTreeWidgetItem{name};
		uuid->setText(0, tr("ID"));
		uuid->setText(1, kvpp::DMXValue::idToString(kvpp::DMXValue::ID::UUID).c_str());
		uuid->setText(2, QByteArray{reinterpret_cast<const char*>(element.getUUID().data()), static_cast<qsizetype>(element.getUUID().size())}.toHex());

		for (const auto& attribute : element.getAttributes()) {
			auto* item = new QTreeWidgetItem{name};
			item->setText(0, attribute.getKey().data());
			item->setText(1, kvpp::DMXValue::idToString(attribute.getValueType()).c_str());
			item->setText(2, attribute.getValueString().c_str());

			if (std::ranges::find(seenElements, elementIndex) != seenElements.end()) {
				auto* dotdotdot = new QTreeWidgetItem{item};
				dotdotdot->setText(0, "...");
				continue;
			}

			if (attribute.getValueType() == kvpp::DMXValue::ID::ELEMENT) {
				if (const auto elementValue = attribute.getValue<kvpp::DMXValue::Element>(); elementValue.index != -2 && elementValue.index < elements.size()) {
					if (parent != root) {
						seenElements.push_back(elementIndex);
					}
					addElement(elementValue.index, item, seenElements);
					std::erase(seenElements, elementIndex);
				}
			} else if (attribute.getValueType() == kvpp::DMXValue::ID::ARRAY_ELEMENT) {
				for (const auto& elementValue : attribute.getValue<std::vector<kvpp::DMXValue::Element>>()) {
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
	return {};
}
