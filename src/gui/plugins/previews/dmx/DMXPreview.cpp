// ReSharper disable CppDFAMemoryLeak
// ReSharper disable CppUseStructuredBinding

#include "DMXPreview.h"

#include <algorithm>
#include <functional>
#include <span>

#include <dmxpp/dmxpp.h>
#include <QTreeWidget>

using namespace dmxpp;

void DMXPreview::initPlugin(IVPKEditPreviewPlugin_V1_0_IWindowAccess*) {}

void DMXPreview::initPreview(QWidget* parent) {
	this->preview = new QTreeWidget{parent};
	this->preview->setColumnCount(3);
	this->preview->setColumnWidth(0, 250);
	this->preview->setColumnWidth(1, 75);
	this->preview->setHeaderLabels({tr("Key"), tr("Value Type"), tr("Value")});
}

QWidget* DMXPreview::getPreview() const {
	return this->preview;
}

QIcon DMXPreview::getIcon() const {
	// todo: cool icon
	return {};
}

IVPKEditPreviewPlugin_V1_0::Error DMXPreview::setData(const QString&, const quint8* dataPtr, quint64 length) {
	this->preview->clear();

	std::unique_ptr<DMX> dmx;
	try {
		dmx = std::make_unique<DMX>(dataPtr, length);
	} catch (const std::overflow_error&) {}
	if (!dmx) {
		emit this->showInfoPreview({":/icons/error.png"}, tr("Failed to parse DMX file."));
		return ERROR_SHOWED_OTHER_PREVIEW;
	}

	auto* root = new QTreeWidgetItem{this->preview};
	root->setText(0, QString{"%1 v%2"}.arg(QString{dmx->getFormatType().data()}.toUpper()).arg(dmx->getFormatVersion()));

	const auto& elements = dmx->getElements();
	QVector<std::array<std::byte, 16>> referencedElements;

	std::function<void(int, QTreeWidgetItem*, std::vector<int>&)> addElement;
	addElement = [&](int elementIndex, QTreeWidgetItem* parent, std::vector<int>& seenElements) {
		const auto& element = elements[elementIndex];

		if (parent == root && std::ranges::find(referencedElements, element.guid) != referencedElements.end()) {
			return;
		}
		referencedElements.push_back(element.guid);

		auto* name = new QTreeWidgetItem{parent};
		name->setText(0, element.name.c_str());
		auto* type = new QTreeWidgetItem{name};
		type->setText(0, tr("Type"));
		type->setText(1, Value::IDToString(Value::ID::STRING).c_str());
		type->setText(2, element.type.c_str());
		auto* guid = new QTreeWidgetItem{name};
		guid->setText(0, tr("GUID"));
		guid->setText(1, Value::IDToString(Value::ID::BYTEARRAY).c_str());
		guid->setText(2, QByteArray{reinterpret_cast<const char*>(element.guid.data()), static_cast<qsizetype>(element.guid.size())}.toHex());

		auto* attributeParent = new QTreeWidgetItem{name};
		attributeParent->setText(0, tr("Attributes"));
		for (const auto& attribute : element.attributes) {
			auto* item = new QTreeWidgetItem{attributeParent};
			item->setText(0, attribute.name.c_str());
			item->setText(1, Value::IDToString(attribute.type).c_str());
			item->setText(2, attribute.getValue().c_str());

			if (std::ranges::find(seenElements, elementIndex) != seenElements.end()) {
				auto* dotdotdot = new QTreeWidgetItem{item};
				dotdotdot->setText(0, "...");
				continue;
			}

			if (attribute.type == Value::ID::ELEMENT) {
				if (const auto elementValue = attribute.getValueAs<Value::Element>(); elementValue.index != -2 && elementValue.index < elements.size()) {
					if (parent != root) {
						seenElements.push_back(elementIndex);
					}
					addElement(static_cast<int>(elementValue.index), item, seenElements);
					std::erase(seenElements, elementIndex);
				}
			} else if (attribute.type == Value::ID::ARRAY_ELEMENT) {
				for (const auto elementsValue = attribute.getValueAs<std::vector<Value::Element>>(); const auto& elementValue : elementsValue) {
					if (elementValue.index != -2 && elementValue.index < elements.size()) {
						if (parent != root) {
							seenElements.push_back(elementIndex);
						}
						addElement(static_cast<int>(elementValue.index), item, seenElements);
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
	return ERROR_SHOWED_THIS_PREVIEW;
}
