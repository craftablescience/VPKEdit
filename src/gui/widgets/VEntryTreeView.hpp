#pragma once

#include <atomic>
#include <memory>
#include <variant>

#include <QApplication>
#include <QPluginLoader>
#include <QTreeView>
#include <vpkpp/PackFile.h>

#include "plugins/thumbnails/IVPKEditThumbnailPlugin.hpp"

class QThread;

class VEntryTreeItem {
public:
	using Data = std::variant<QString, std::shared_ptr<vpkpp::PackFile>>;

	explicit VEntryTreeItem(Data data_, VEntryTreeItem* parent_ = nullptr) : data{std::move(data_)}, parent{parent_} {}

	template<typename Self>
	[[nodiscard]] auto* child(this Self&& self, int row) {
		return row >= 0 && row < self.childCount() ? self.children.at(row).get() : nullptr;
	}

	[[nodiscard]] int childCount() const;

	[[nodiscard]] QString filename() const;

	[[nodiscard]] QString path() const;

	[[nodiscard]] bool isPackFile() const;

	template<typename Self>
	[[nodiscard]] std::weak_ptr<vpkpp::PackFile> packFile(this Self&& self) {
		auto* item = &self;
		do {
			if (item->isPackFile()) {
				return std::get<std::shared_ptr<vpkpp::PackFile>>(item->data);
			}
		} while ((item = item->parent));
		return {};
	}

	[[nodiscard]] bool isDirectory() const;

	[[nodiscard]] int row() const;

	template<typename Self>
	[[nodiscard]] auto* parentItem(this Self&& self) {
		return self.parent;
	}

	VEntryTreeItem* addChild(Data data_);

	void sort(Qt::SortOrder order = Qt::AscendingOrder);

private:
	Data data;
	std::vector<std::unique_ptr<VEntryTreeItem>> children;
	VEntryTreeItem* parent;
};

class VEntryTreeModel : public QAbstractItemModel {
	Q_OBJECT;

public:
	Q_DISABLE_COPY_MOVE(VEntryTreeModel);

	explicit VEntryTreeModel(QObject* parent = nullptr) : QAbstractItemModel{parent}, rootItem{std::make_unique<VEntryTreeItem>("__root__", nullptr)} {}

	template<typename Self>
	[[nodiscard]] auto* root(this Self&& self) {
		return self.rootItem.get();
	}

	[[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;

	[[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;

	[[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override; // NOLINT(*-default-arguments)

	[[nodiscard]] QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override; // NOLINT(*-default-arguments)

	[[nodiscard]] QModelIndex parent(const QModelIndex& index) const override;

	[[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override; // NOLINT(*-default-arguments)

	[[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override; // NOLINT(*-default-arguments)

	void addPackFile(const QString& path);

private:
	std::unique_ptr<VEntryTreeItem> rootItem;
};

class VEntryTreeThumbnailProvider;

class VEntryTreeView : public QTreeView {
	Q_OBJECT;

public:
	explicit VEntryTreeView(QWidget* parent = nullptr);

	template<typename Self>
	[[nodiscard]] auto* thumbnailProvider(this Self&& self) {
		return self.thumbnailProvider_;
	}

	[[nodiscard]] static const QIcon& cachedDirIcon();

	[[nodiscard]] static const QIcon& cachedFileIcon(const QString& path);

signals:
	void dirSelected(const QString& path, vpkpp::Attribute attrs, const QStringList& subdirs, const QMap<QString, std::optional<vpkpp::Entry>>& subfiles);

	void fileSelected(const QString& path, const QByteArray& data);

public slots:
	//void selectEntry(QString path);

	void addPackFile(const QString& path) const;

	void onThemeUpdated() const;

protected:
	VEntryTreeThumbnailProvider* thumbnailProvider_;
};

class VEntryTreeThumbnailProvider : public QObject {
	Q_OBJECT;

public:
	explicit VEntryTreeThumbnailProvider(QObject* parent = nullptr);

signals:
	void processedThumbnail(const QString& path, const QIcon& thumbnail);

public slots:
	void startProcessingThumbnails(std::shared_ptr<vpkpp::PackFile> packFile, QString packFilePath, QStringList paths, uint16_t size);

protected:
	QThread* processingThread;

	QList<QPluginLoader*> thumbnailPluginLoaders;
	QMap<QString, IVPKEditThumbnailPlugin_V6_0*> thumbnailPlugins;
};

class VEntryTreeThumbnailProviderWorker : public QObject {
	Q_OBJECT;

public:
	using QObject::QObject;

signals:
	void processedThumbnail(const QString& path, const QIcon& thumbnail);

	void processingComplete();

public slots:
	void processThumbnails(const std::shared_ptr<vpkpp::PackFile>& packFile, const QString& packFilePath, const QStringList& paths, uint16_t size, const QMap<QString, IVPKEditThumbnailPlugin_V6_0*>& thumbnailPlugins);

	void cancel();

protected:
	std::atomic<bool> cancelPending = false;
};
