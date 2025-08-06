#include "DMXPreview.h"

#include <algorithm>
#include <functional>

#include <dmxpp/dmxpp.h>

#include "../FileViewer.h"

using namespace dmxpp;

DMXPreview::DMXPreview(FileViewer* fileViewer_, QWidget* parent)
		: QTreeWidget(parent)
		, fileViewer(fileViewer_) {
	this->setColumnCount(3);
	this->setColumnWidth(0, 250);
	this->setColumnWidth(1, 75);
	this->setHeaderLabels({tr("Key"), tr("Value Type"), tr("Value")});
}

void DMXPreview::setData(const std::vector<std::byte>& data) {
	this->clear();

	std::unique_ptr<DMX> dmx;
	try {
		dmx = std::make_unique<DMX>(data);
	} catch (const std::overflow_error&) {}
	if (!dmx) {
		this->fileViewer->showInfoPreview({":/icons/error.png"}, tr("Failed to parse DMX file."));
		return;
	}

	auto* root = new QTreeWidgetItem(this);
	root->setText(0, QString("%1 v%2").arg(QString(dmx->getFormatType().data()).toUpper()).arg(dmx->getFormatVersion()));

	const auto& elements = dmx->getElements();
	QVector<std::array<std::byte, 16>> referencedElements;

	std::function<void(int, QTreeWidgetItem*, std::vector<int>&)> addElement;
	addElement = [&](int elementIndex, QTreeWidgetItem* parent, std::vector<int>& seenElements) {
		const auto& element = elements[elementIndex];

		if (parent == root && std::ranges::find(referencedElements, element.guid) != referencedElements.end()) {
			return;
		}
		referencedElements.push_back(element.guid);

		auto* name = new QTreeWidgetItem(parent);
		name->setText(0, element.name.c_str());
		auto* type = new QTreeWidgetItem(name);
		type->setText(0, tr("Type"));
		type->setText(1, Value::IDToString(Value::ID::STRING).c_str());
		type->setText(2, element.type.c_str());
		auto* guid = new QTreeWidgetItem(name);
		guid->setText(0, tr("GUID"));
		guid->setText(1, Value::IDToString(Value::ID::BYTEARRAY).c_str());
		guid->setText(2, QByteArray(reinterpret_cast<const char*>(element.guid.data()), static_cast<qsizetype>(element.guid.size())).toHex());

		auto* attributeParent = new QTreeWidgetItem(name);
		attributeParent->setText(0, tr("Attributes"));
		for (const auto& attribute : element.attributes) {
			auto* item = new QTreeWidgetItem(attributeParent);
			item->setText(0, attribute.name.c_str());
			item->setText(1, Value::IDToString(attribute.type).c_str());
			item->setText(2, attribute.getValue().c_str());

			if (std::find(seenElements.begin(), seenElements.end(), elementIndex) != seenElements.end()) {
				auto* dotdotdot = new QTreeWidgetItem(item);
				dotdotdot->setText(0, "...");
				continue;
			}

			if (attribute.type == Value::ID::ELEMENT) {
				auto elementValue = attribute.getValueAs<Value::Element>();
				if (elementValue.index != -2 && elementValue.index < elements.size()) {
					if (parent != root) {
						seenElements.push_back(elementIndex);
					}
					addElement(static_cast<int>(elementValue.index), item, seenElements);
					std::erase(seenElements, elementIndex);
				}
			} else if (attribute.type == Value::ID::ARRAY_ELEMENT) {
				auto elementsValue = attribute.getValueAs<std::vector<Value::Element>>();
				for (const auto& elementValue : elementsValue) {
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
}
