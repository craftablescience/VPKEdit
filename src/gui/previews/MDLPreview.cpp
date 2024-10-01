#include "MDLPreview.h"

#include <filesystem>
#include <utility>

#include <kvpp/kvpp.h>
#include <mdlpp/mdlpp.h>
#include <QApplication>
#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QSpinBox>
#include <QStyleOption>
#include <QTabWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QtMath>
#include <sourcepp/String.h>
#include <vpkpp/vpkpp.h>
#include <vtfpp/vtfpp.h>

#include "../utility/ThemedIcon.h"
#include "../FileViewer.h"
#include "../Window.h"

using namespace kvpp;
using namespace mdlpp;
using namespace sourcepp;
using namespace std::literals;
using namespace vpkpp;
using namespace vtfpp;

namespace {

std::unique_ptr<MDLTextureData> getTextureDataForMaterial(PackFile& packFile, const std::string& materialPath) {
	auto materialFile = packFile.readEntryText(materialPath);
	if (!materialFile) {
		return nullptr;
	}

	KV1 materialKV{*materialFile};
	if (materialKV.getChildCount() == 0) {
		return nullptr;
	}

	auto& baseTexturePathKV = materialKV[0]["$basetexture"];
	if (baseTexturePathKV.isInvalid()) {
		return nullptr;
	}

	auto textureFile = packFile.readEntry("materials/" + std::string{baseTexturePathKV.getValue()} + ".vtf");
	if (!textureFile) {
		return nullptr;
	}

	VTF vtf{*textureFile};
	return std::make_unique<MDLTextureData>(
			vtf.getImageDataAs(ImageFormat::RGB888),
			vtf.getWidth(),
			vtf.getHeight()
	);
}

} // namespace

QList<QVector3D> AABB::getCorners() const {
	return {
		{this->min.x(), this->min.y(), this->min.z()},
		{this->max.x(), this->min.y(), this->min.z()},
		{this->min.x(), this->max.y(), this->min.z()},
		{this->min.x(), this->min.y(), this->max.z()},
		{this->max.x(), this->max.y(), this->max.z()},
		{this->min.x(), this->max.y(), this->max.z()},
		{this->max.x(), this->min.y(), this->max.z()},
		{this->max.x(), this->max.y(), this->min.z()},
	};
}

float AABB::getWidth() const {
	return this->max.x() - this->min.x();
}

float AABB::getHeight() const {
	return this->max.y() - this->min.y();
}

float AABB::getDepth() const {
	return this->max.z() - this->min.z();
}

MDLWidget::MDLWidget(QWidget* parent)
		: QOpenGLWidget(parent)
		, QOpenGLFunctions_3_2_Core()
		, missingTexture(QOpenGLTexture::Target2D)
		, matCapTexture(QOpenGLTexture::Target2D)
		, vertexCount(0)
		, skin(0)
		, shadingMode(MDLShadingMode::UNSHADED_TEXTURED)
		, distance(0.0)
		, distanceScale(0.0)
		, fov(70.0)
		, cullBackFaces(true)
		, angularSpeed(0.0)
		, rmbBeingHeld(false) {}

MDLWidget::~MDLWidget() {
	this->clearMeshes();
	if (this->missingTexture.isCreated()) {
		this->missingTexture.destroy();
	}
	if (this->matCapTexture.isCreated()) {
		this->matCapTexture.destroy();
	}
}

void MDLWidget::setModel(const BakedModel& model) {
	// Set vertex data
	if (this->vertices.isCreated()) {
		this->vertices.destroy();
	}
	this->vertexCount = static_cast<int>(model.vertices.size());
	this->vertices.create();
	this->vertices.bind();
	this->vertices.setUsagePattern(QOpenGLBuffer::StaticDraw);
	this->vertices.allocate(model.vertices.data(), static_cast<int>(this->vertexCount * sizeof(BakedModel::Vertex)));
	this->vertices.release();

	// Add meshes
	for (const auto& bakedMesh : model.meshes) {
		auto& mesh = this->meshes.emplace_back();

		mesh.textureIndex = bakedMesh.materialIndex;

		mesh.indexCount = static_cast<int>(bakedMesh.indices.size());
		mesh.ebo.create();
		mesh.ebo.bind();
		mesh.ebo.allocate(bakedMesh.indices.data(), static_cast<int>(mesh.indexCount * sizeof(uint16_t)));
		mesh.ebo.release();
	}
}

void MDLWidget::setTextures(const std::vector<std::unique_ptr<MDLTextureData>>& vtfData) {
	this->clearTextures();
	for (const auto& vtf : vtfData) {
		if (!vtf) {
			this->textures.push_back(nullptr);
			continue;
		}
		auto* texture = new QOpenGLTexture(QOpenGLTexture::Target::Target2D);
		texture->create();
		texture->setData(QImage(reinterpret_cast<uchar*>(vtf->data.data()), static_cast<int>(vtf->width), static_cast<int>(vtf->height), QImage::Format_RGB888));
		this->textures.push_back(texture);
	}
}

void MDLWidget::clearTextures() {
	for (auto* texture : this->textures) {
		if (texture && texture->isCreated()) {
			texture->destroy();
		}
		delete texture;
	}
	this->textures.clear();
}

void MDLWidget::setSkinLookupTable(std::vector<std::vector<short>> skins_) {
	this->skins = std::move(skins_);
}

void MDLWidget::setAABB(AABB aabb) {
	// https://stackoverflow.com/a/32836605 - calculate optimal camera distance from bounding box
	auto midpoint = (aabb.max + aabb.min) / 2.0f;
	float sphereRadius = 0.0f;
	for (const auto corner : aabb.getCorners()) {
		if (auto dist = midpoint.distanceToPoint(corner); dist > sphereRadius) {
			sphereRadius = dist;
		}
	}
	float fovRad = qDegreesToRadians(this->fov);
	this->target = midpoint;
	this->distance = static_cast<float>(sphereRadius / qTan(fovRad / 2));
	this->distanceScale = this->distance / 128.0f;
}

void MDLWidget::setSkin(int skin_) {
	this->skin = skin_;
	this->update();
}

void MDLWidget::setShadingMode(MDLShadingMode type) {
	this->shadingMode = type;
	this->update();
}

void MDLWidget::setFieldOfView(float newFOV) {
	this->fov = newFOV;
	this->update();
}

void MDLWidget::setCullBackFaces(bool enable) {
	this->cullBackFaces = enable;
	this->update();
}

void MDLWidget::clearMeshes() {
	if (this->vertices.isCreated()) {
		this->vertices.destroy();
	}

	this->clearTextures();

	for (auto& mesh : this->meshes) {
		if (mesh.ebo.isCreated()) {
			mesh.ebo.destroy();
		}
	}
	this->meshes.clear();

	this->skin = 0;
	this->skins.clear();

	this->update();
}

void MDLWidget::initializeGL() {
	if (!this->initializeOpenGLFunctions()) {
		QMessageBox::critical(this, tr("Error"), tr("Unable to initialize OpenGL 3.2 Core context! Please upgrade your computer to preview models."));
		return; // and probably crash right after
	}

	this->wireframeShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/mdl.vert");
	this->wireframeShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/mdl_wireframe.frag");
	this->wireframeShaderProgram.link();

	this->shadedUntexturedShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/mdl.vert");
	this->shadedUntexturedShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/mdl_shaded_untextured.frag");
	this->shadedUntexturedShaderProgram.link();

	this->unshadedTexturedShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/mdl.vert");
	this->unshadedTexturedShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/mdl_unshaded_textured.frag");
	this->unshadedTexturedShaderProgram.link();

	this->shadedTexturedShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/mdl.vert");
	this->shadedTexturedShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/mdl_shaded_textured.frag");
	this->shadedTexturedShaderProgram.link();

	this->missingTexture.create();
	this->missingTexture.setData(QImage(":/textures/checkerboard.png"));

	this->matCapTexture.create();
	this->matCapTexture.setData(QImage(":/textures/default_matcap.png"));

	this->timer.start(12, this);
}

void MDLWidget::resizeGL(int w, int h) {
	this->glViewport(0, 0, w, h);

	const float aspectRatio = static_cast<float>(w) / static_cast<float>(h > 0 ? h : 1);
	const float nearPlane = 0.015f, farPlane = 32768.0f;
	this->projection.setToIdentity();
	this->projection.perspective(this->fov, aspectRatio, nearPlane, farPlane);
}

void MDLWidget::paintGL() {
	QStyleOption opt;
	opt.initFrom(this);

	auto clearColor = opt.palette.color(QPalette::ColorRole::Window);
	this->glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF());
	this->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (this->meshes.empty()) {
		return;
	}

	this->glEnable(GL_MULTISAMPLE);
	this->glEnable(GL_DEPTH_TEST);

	if (!this->cullBackFaces || this->shadingMode == MDLShadingMode::WIREFRAME) {
		this->glDisable(GL_CULL_FACE);
	} else {
		this->glEnable(GL_CULL_FACE);
	}

	if (this->shadingMode == MDLShadingMode::WIREFRAME) {
		this->glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		this->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	QOpenGLShaderProgram* currentShaderProgram = nullptr;
	switch (this->shadingMode) {
		case MDLShadingMode::WIREFRAME:
			currentShaderProgram = &this->wireframeShaderProgram;
			break;
		case MDLShadingMode::SHADED_UNTEXTURED:
			currentShaderProgram = &this->shadedUntexturedShaderProgram;
			break;
		case MDLShadingMode::UNSHADED_TEXTURED:
			currentShaderProgram = &this->unshadedTexturedShaderProgram;
			break;
		case MDLShadingMode::SHADED_TEXTURED:
			currentShaderProgram = &this->shadedTexturedShaderProgram;
			break;
	}
	if (!currentShaderProgram) {
		return;
	}
	currentShaderProgram->bind();

	QMatrix4x4 view;
	QVector3D translation(this->target.x(), this->target.y(), -this->target.z() - this->distance);
	view.translate(translation);
	view.rotate(this->rotation);
	currentShaderProgram->setUniformValue("uMVP", this->projection * view);
	currentShaderProgram->setUniformValue("uMV", view);
	currentShaderProgram->setUniformValue("uNormalMatrix", view.normalMatrix());
	currentShaderProgram->setUniformValue("uEyePosition", translation);
	currentShaderProgram->setUniformValue("uMeshTexture", 0);
	currentShaderProgram->setUniformValue("uMatCapTexture", 1);

	this->vertices.bind();

	for (auto& mesh : this->meshes) {
		QOpenGLTexture* texture;
		if (mesh.textureIndex < 0 || this->skins.size() <= this->skin || this->skins[this->skin].size() <= mesh.textureIndex || !(texture = this->textures[this->skins[this->skin][mesh.textureIndex]])) {
			texture = &this->missingTexture;
		}
		texture->bind(0);

		this->matCapTexture.bind(1);

		mesh.ebo.bind();

		int offset = 0;
		int vertexPosLocation = currentShaderProgram->attributeLocation("vPos");
		currentShaderProgram->enableAttributeArray(vertexPosLocation);
		currentShaderProgram->setAttributeBuffer(vertexPosLocation, GL_FLOAT, offset, 3, sizeof(BakedModel::Vertex));
		offset += sizeof(math::Vec3f);

		int vertexNormalLocation = currentShaderProgram->attributeLocation("vNormal");
		currentShaderProgram->enableAttributeArray(vertexNormalLocation);
		currentShaderProgram->setAttributeBuffer(vertexNormalLocation, GL_FLOAT, offset, 3, sizeof(BakedModel::Vertex));
		offset += sizeof(math::Vec3f);

		int vertexUVLocation = currentShaderProgram->attributeLocation("vUV");
		currentShaderProgram->enableAttributeArray(vertexUVLocation);
		currentShaderProgram->setAttributeBuffer(vertexUVLocation, GL_FLOAT, offset, 2, sizeof(BakedModel::Vertex));

		this->glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_SHORT, nullptr);
		mesh.ebo.release();

		this->matCapTexture.release(1);

		texture->release(0);
	}

	this->vertices.release();

	currentShaderProgram->release();
}

void MDLWidget::mousePressEvent(QMouseEvent* event) {
	this->mousePressPosition = QVector2D(event->position());

	if (event->button() == Qt::MouseButton::RightButton) {
		this->rmbBeingHeld = true;
	}

	event->accept();
}

void MDLWidget::mouseReleaseEvent(QMouseEvent* event) {
	if (event->button() == Qt::MouseButton::RightButton) {
		this->rmbBeingHeld = false;
		event->accept();
	}
}

void MDLWidget::mouseMoveEvent(QMouseEvent* event) {
	QVector2D diff = QVector2D(event->position()) - this->mousePressPosition;

	// If holding shift, just move the mesh
	if (QApplication::queryKeyboardModifiers() & Qt::KeyboardModifier::ShiftModifier) {
		this->translationalVelocity = QVector3D(diff.x() * this->distanceScale / 4.0f, -diff.y() * this->distanceScale / 4.0f, 0.0);
		this->target += this->translationalVelocity;
		this->mousePressPosition = QVector2D(event->position());
		this->update();
		return;
	}

	QVector3D inputAxis;
	if (!this->rmbBeingHeld) {
		// Rotation axis is perpendicular to the mouse position difference vector
		inputAxis = QVector3D(diff.y(), diff.x(), 0.0).normalized();
	} else {
		// Rotation axis is the z-axis
		inputAxis = QVector3D(0.0, 0.0, -diff.x()).normalized();
	}

	// Accelerate relative to the length of the mouse sweep
	float acceleration = diff.length();

	// Update rotation axis, velocity
	this->rotationAxis = (acceleration * inputAxis).normalized();
	this->angularSpeed = acceleration;

	// Update old position
	this->mousePressPosition = QVector2D(event->position());
	this->update();

	event->accept();
}

void MDLWidget::wheelEvent(QWheelEvent* event) {
	if (QPoint numDegrees = event->angleDelta() / 8; !numDegrees.isNull()) {
		this->distance -= static_cast<float>(numDegrees.y()) * this->distanceScale;
		this->update();
	}
	event->accept();
}

constexpr float MOTION_REDUCTION_AMOUNT = 0.75f;

void MDLWidget::timerEvent(QTimerEvent* /*event*/) {
	this->translationalVelocity *= MOTION_REDUCTION_AMOUNT;
	if (this->translationalVelocity.length() < 0.01) {
		this->translationalVelocity = QVector3D();
	} else {
		this->target += this->translationalVelocity;
		this->update();
	}

	this->angularSpeed *= MOTION_REDUCTION_AMOUNT;
	if (this->angularSpeed < 0.01) {
		this->angularSpeed = 0.0;
	} else {
		this->rotation = QQuaternion::fromAxisAndAngle(this->rotationAxis, static_cast<float>(this->angularSpeed)) * this->rotation;
		this->update();
	}
}

constexpr int TOOLBAR_SPACE_SIZE = 48;
constexpr int SHADING_MODE_BUTTON_SIZE = 24;

MDLPreview::MDLPreview(FileViewer* fileViewer_, Window* window, QWidget* parent)
		: QWidget(parent)
		, fileViewer(fileViewer_)
		, shadingModeWireframe(nullptr)
		, shadingModeShadedUntextured(nullptr)
		, shadingModeUnshadedTextured(nullptr)
		, shadingModeShadedTextured(nullptr) {
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);

	auto* controls = new QFrame(this);
	controls->setFrameShape(QFrame::Shape::StyledPanel);
	controls->setFixedHeight(TOOLBAR_SPACE_SIZE);
	layout->addWidget(controls, Qt::AlignRight);

	auto* controlsLayout = new QHBoxLayout(controls);
	controlsLayout->setAlignment(Qt::AlignRight);

	auto* tabsToggleButton = new QPushButton(tr("Toggle Info Panel"), this);
	tabsToggleButton->setCheckable(true);
	tabsToggleButton->setChecked(false);
	QObject::connect(tabsToggleButton, &QPushButton::clicked, this, [&](bool checked) {
		this->tabs->setVisible(checked);
	});
	controlsLayout->addWidget(tabsToggleButton);

	controlsLayout->addSpacing(TOOLBAR_SPACE_SIZE);

	this->backfaceCulling = new QCheckBox(tr("Backface Culling"), this);
	this->backfaceCulling->setCheckState(Qt::CheckState::Checked);
	QObject::connect(this->backfaceCulling, &QCheckBox::stateChanged, this, [&](int state) {
		this->mdl->setCullBackFaces(state == Qt::CheckState::Checked);
	});
	controlsLayout->addWidget(this->backfaceCulling);

	controlsLayout->addSpacing(TOOLBAR_SPACE_SIZE);

	controlsLayout->addWidget(new QLabel(tr("Skin"), this));
	this->skinSpinBox = new QSpinBox(this);
	this->skinSpinBox->setFixedWidth(32);
	this->skinSpinBox->setMinimum(0);
	this->skinSpinBox->setValue(0);
	QObject::connect(this->skinSpinBox, &QSpinBox::valueChanged, this, [&](int value) {
		this->mdl->setSkin(value);
	});
	controlsLayout->addWidget(this->skinSpinBox);

	controlsLayout->addSpacing(TOOLBAR_SPACE_SIZE);

	const QList<QPair<QToolButton**, Qt::Key>> buttons{
		{&this->shadingModeWireframe,        Qt::Key_1},
		{&this->shadingModeShadedUntextured, Qt::Key_2},
		{&this->shadingModeUnshadedTextured, Qt::Key_3},
		{&this->shadingModeShadedTextured,   Qt::Key_4},
	};
	for (int i = 0; i < buttons.size(); i++) {
		auto* button = *(buttons[i].first) = new QToolButton(this);
		button->setToolButtonStyle(Qt::ToolButtonIconOnly);
		button->setFixedSize(SHADING_MODE_BUTTON_SIZE, SHADING_MODE_BUTTON_SIZE);
		button->setStyleSheet(
				"QToolButton          { background-color: rgba(0,0,0,0); border: none; }\n"
				"QToolButton::pressed { background-color: rgba(0,0,0,0); border: none; }");
		button->setShortcut(buttons[i].second);
		QObject::connect(button, &QToolButton::pressed, this, [this, i] {
			this->setShadingMode(static_cast<MDLShadingMode>(i));
		});
		controlsLayout->addWidget(button, 0, Qt::AlignVCenter | Qt::AlignRight);
	}

	this->mdl = new MDLWidget(this);
	layout->addWidget(this->mdl);

	this->tabs = new QTabWidget(this);
	this->tabs->setFixedHeight(150);
	this->tabs->hide();

	this->materialsTab = new QTreeWidget(this->tabs);
	this->materialsTab->setHeaderHidden(true);
	QObject::connect(this->materialsTab, &QTreeWidget::itemClicked, this, [window](QTreeWidgetItem* item) {
		window->selectEntryInEntryTree(item->text(0));
	});
	this->tabs->addTab(this->materialsTab, tr("Materials Found"));

	this->allMaterialsTab = new QTreeWidget(this->tabs);
	this->allMaterialsTab->setHeaderHidden(true);
	this->tabs->addTab(this->allMaterialsTab, tr("All Materials"));

	layout->addWidget(this->tabs);
}

void MDLPreview::setMesh(const QString& path, PackFile& packFile) const {
	this->mdl->clearMeshes();

	std::string basePath = std::filesystem::path{path.toLocal8Bit().constData()}.replace_extension().string();
	if (path.endsWith(".vtx")) {
		// Remove .dx80, .dx90, .sw
		basePath = std::filesystem::path{basePath}.replace_extension().string();
	}

	auto mdlData = packFile.readEntry(basePath + ".mdl");
	auto vvdData = packFile.readEntry(basePath + ".vvd");
	auto vtxData = packFile.readEntry(basePath + ".vtx");
	if (!vtxData) {
		vtxData = packFile.readEntry(basePath + ".dx90.vtx");
	}
	if (!vtxData) {
		vtxData = packFile.readEntry(basePath + ".dx80.vtx");
	}
	if (!vtxData) {
		vtxData = packFile.readEntry(basePath + ".sw.vtx");
	}
	if (!mdlData || !vvdData || !vtxData) {
		QString error{tr("Unable to find all the required files the model is composed of!") + '\n'};
		if (!mdlData) {
			error += "\n- ";
			error += basePath.c_str();
			error += ".mdl";
		}
		if (!vvdData) {
			error += "\n- ";
			error += basePath.c_str();
			error += ".vvd";
		}
		if (!vtxData) {
			error += "\n- " + tr("One of the following:") +
					 "\n  - " + basePath.c_str() + ".vtx" +
					 "\n  - " + basePath.c_str() + ".dx90.vtx" +
					 "\n  - " + basePath.c_str() + ".dx80.vtx" +
					 "\n  - " + basePath.c_str() + ".sw.vtx";
		}

		this->fileViewer->showGenericErrorPreview(error);
		return;
	}

	StudioModel mdlParser;
	bool opened = mdlParser.open(reinterpret_cast<const uint8_t*>(mdlData->data()), mdlData->size(),
								 reinterpret_cast<const uint8_t*>(vtxData->data()), vtxData->size(),
								 reinterpret_cast<const uint8_t*>(vvdData->data()), vvdData->size());
	if (!opened) {
		this->fileViewer->showGenericErrorPreview(tr("This model is invalid, it cannot be previewed!"));
		return;
	}

	// Maybe we can add a setting for LOD...
	auto bakedModel = mdlParser.processModelData(ROOT_LOD);
	this->mdl->setModel(bakedModel);

	this->skinSpinBox->setValue(0);
	this->skinSpinBox->setMaximum(std::max(static_cast<int>(mdlParser.mdl.skins.size()) - 1, 0));
	this->skinSpinBox->setDisabled(this->skinSpinBox->maximum() == 0);
	this->mdl->setSkinLookupTable(mdlParser.mdl.skins);

	this->mdl->setAABB({
		{mdlParser.mdl.hullMin[0], mdlParser.mdl.hullMin[1], mdlParser.mdl.hullMin[2]},
		{mdlParser.mdl.hullMax[0], mdlParser.mdl.hullMax[1], mdlParser.mdl.hullMax[2]},
	});

	// Add material directories and names to the material names panel
	this->allMaterialsTab->clear();
	auto* allMaterialDirsItem = new QTreeWidgetItem(this->allMaterialsTab);
	allMaterialDirsItem->setText(0, tr("Folders"));
	this->allMaterialsTab->addTopLevelItem(allMaterialDirsItem);
	for (const auto& materialDir : mdlParser.mdl.materialDirectories) {
		auto* materialDirItem = new QTreeWidgetItem(allMaterialDirsItem);
		materialDirItem->setText(0, materialDir.c_str());
	}
	allMaterialDirsItem->setExpanded(true);
	auto* allMaterialNamesItem = new QTreeWidgetItem(this->allMaterialsTab);
	allMaterialNamesItem->setText(0, tr("Material Names"));
	this->allMaterialsTab->addTopLevelItem(allMaterialNamesItem);
	for (const auto& material : mdlParser.mdl.materials) {
		auto* materialNameItem = new QTreeWidgetItem(allMaterialNamesItem);
		materialNameItem->setText(0, material.name.c_str());
	}
	allMaterialNamesItem->setExpanded(true);

	// Add the materials that actually exist to the found materials panel
	this->materialsTab->clear();
	std::vector<std::unique_ptr<MDLTextureData>> vtfs;
	bool foundAnyMaterials = false;
	for (int materialIndex = 0; materialIndex < mdlParser.mdl.materials.size(); materialIndex++) {
		bool foundMaterial = false;
		for (int materialDirIndex = 0; materialDirIndex < mdlParser.mdl.materialDirectories.size(); materialDirIndex++) {
			std::string vmtPath = "materials/"s + mdlParser.mdl.materialDirectories.at(materialDirIndex) + mdlParser.mdl.materials.at(materialIndex).name + ".vmt";
			string::normalizeSlashes(vmtPath);
			if (auto data = getTextureDataForMaterial(packFile, vmtPath)) {
				vtfs.push_back(std::move(data));

				auto* item = new QTreeWidgetItem(this->materialsTab);
				item->setText(0, vmtPath.c_str());
				this->materialsTab->addTopLevelItem(item);

				foundMaterial = true;
				break;
			}
		}
		if (!foundMaterial) {
			vtfs.emplace_back(nullptr);
		}
		foundAnyMaterials = foundAnyMaterials || foundMaterial;
	}
	this->mdl->setTextures(vtfs);

	if (foundAnyMaterials) {
		this->setShadingMode(MDLShadingMode::SHADED_TEXTURED);
	} else {
		this->setShadingMode(MDLShadingMode::SHADED_UNTEXTURED);
	}
	this->mdl->update();
}

void MDLPreview::setShadingMode(MDLShadingMode mode) const {
	this->backfaceCulling->setDisabled(mode == MDLShadingMode::WIREFRAME);

	const QList<std::tuple<QToolButton* const*, QString, MDLShadingMode>> buttonsAndIcons{
			{&this->shadingModeWireframe, ":/icons/model_wireframe.png", MDLShadingMode::WIREFRAME},
			{&this->shadingModeShadedUntextured, ":/icons/model_shaded_untextured.png", MDLShadingMode::SHADED_UNTEXTURED},
			{&this->shadingModeUnshadedTextured, ":/icons/model_unshaded_textured.png", MDLShadingMode::UNSHADED_TEXTURED},
			{&this->shadingModeShadedTextured, ":/icons/model_shaded_textured.png", MDLShadingMode::SHADED_TEXTURED},
	};
	for (auto& [button, iconPath, buttonMode] : buttonsAndIcons) {
		(*button)->setIcon(ThemedIcon::get(this, iconPath, buttonMode == mode ? QPalette::ColorRole::Link : QPalette::ColorRole::ButtonText));
		(*button)->setIconSize({SHADING_MODE_BUTTON_SIZE, SHADING_MODE_BUTTON_SIZE});
	}

	this->mdl->setShadingMode(mode);
}
