#include "MDLPreview.h"

#include <filesystem>
#include <optional>
#include <tuple>

#include <KeyValue.h>
#include <studiomodelpp/studiomodelpp.h>
#include <QApplication>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyleOption>
#include <QtMath>
#include <vpkedit/VPK.h>
#include <VTFLib.h>

#include "../FileViewer.h"
#include "info/FileLoadErrorPreview.h"
#include "info/InvalidMDLErrorPreview.h"

using namespace std::literals;
using namespace studiomodelpp;
using namespace vpkedit;

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

void MDLWidget::setVertices(const QVector<MDLVertex>& vertices_) {
	if (this->vertices.isCreated()) {
		this->vertices.destroy();
	}
	this->vertexCount = static_cast<int>(vertices_.size());
	this->vertices.create();
	this->vertices.bind();
	this->vertices.setUsagePattern(QOpenGLBuffer::StaticDraw);
	this->vertices.allocate(vertices_.constData(), static_cast<int>(this->vertexCount * sizeof(MDLVertex)));
	this->vertices.release();
}

void MDLWidget::addSubMesh(const QVector<unsigned short>& indices) {
	auto& mesh = this->meshes.emplace_back();

	mesh.texture = nullptr;

	mesh.indexCount = static_cast<int>(indices.size());
	mesh.ebo.create();
	mesh.ebo.bind();
	mesh.ebo.allocate(indices.constData(), static_cast<int>(mesh.indexCount * sizeof(unsigned short)));
	mesh.ebo.release();
}

void MDLWidget::addSubMesh(const QVector<unsigned short>& indices, VTFData&& vtfData) {
	auto& mesh = this->meshes.emplace_back();

	mesh.vtfData = std::move(vtfData);
	mesh.texture = new QOpenGLTexture{QOpenGLTexture::Target::Target2D};
	mesh.texture->create();
	mesh.texture->setData(QImage(reinterpret_cast<uchar*>(mesh.vtfData.data.get()), static_cast<int>(mesh.vtfData.width), static_cast<int>(mesh.vtfData.height), mesh.vtfData.format));

	mesh.indexCount = static_cast<int>(indices.size());
	mesh.ebo.create();
	mesh.ebo.bind();
	mesh.ebo.allocate(indices.constData(), static_cast<int>(mesh.indexCount * sizeof(unsigned short)));
	mesh.ebo.release();
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

	for (auto& mesh : this->meshes) {
		if (mesh.texture && mesh.texture->isCreated()) {
			mesh.texture->destroy();
		}
		delete mesh.texture;

		if (mesh.ebo.isCreated()) {
			mesh.ebo.destroy();
		}
	}
	this->meshes.clear();

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
		auto* texture = mesh.texture;
		if (!texture) {
			texture = &this->missingTexture;
		}
		texture->bind(0);

		this->matCapTexture.bind(1);

		mesh.ebo.bind();

		int offset = 0;
		int vertexPosLocation = currentShaderProgram->attributeLocation("vPos");
		currentShaderProgram->enableAttributeArray(vertexPosLocation);
		currentShaderProgram->setAttributeBuffer(vertexPosLocation, GL_FLOAT, offset, 3, sizeof(MDLVertex));

		offset += sizeof(QVector3D);
		int vertexNormalLocation = currentShaderProgram->attributeLocation("vNormal");
		currentShaderProgram->enableAttributeArray(vertexNormalLocation);
		currentShaderProgram->setAttributeBuffer(vertexNormalLocation, GL_FLOAT, offset, 3, sizeof(MDLVertex));

		offset += sizeof(QVector3D);
		int vertexUVLocation = currentShaderProgram->attributeLocation("vUV");
		currentShaderProgram->enableAttributeArray(vertexUVLocation);
		currentShaderProgram->setAttributeBuffer(vertexUVLocation, GL_FLOAT, offset, 2, sizeof(MDLVertex));

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
}

void MDLWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MouseButton::RightButton) {
        this->rmbBeingHeld = false;
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
    if (!rmbBeingHeld) {
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

constexpr int SHADING_MODE_BUTTON_SIZE = 24;

MDLPreview::MDLPreview(FileViewer* fileViewer_, QWidget* parent)
		: QWidget(parent)
		, fileViewer(fileViewer_)
		, shadingModeWireframe(nullptr)
		, shadingModeShadedUntextured(nullptr)
		, shadingModeUnshadedTextured(nullptr)
		, shadingModeShadedTextured(nullptr) {
	auto* layout = new QVBoxLayout(this);

    auto* controls = new QWidget(this);
	controls->setFixedHeight(34);
	layout->addWidget(controls, Qt::AlignRight);

    auto* controlsLayout = new QHBoxLayout(controls);
	controlsLayout->setAlignment(Qt::AlignRight);

	this->backfaceCulling = new QCheckBox(tr("Backface Culling"), this);
	this->backfaceCulling->setCheckState(Qt::CheckState::Checked);
	QObject::connect(this->backfaceCulling, &QCheckBox::stateChanged, [&](int state) {
		this->mdl->setCullBackFaces(state == Qt::CheckState::Checked);
	});
	controlsLayout->addWidget(this->backfaceCulling, Qt::AlignVCenter | Qt::AlignLeft);

	const QList<QPushButton**> buttons{
		&this->shadingModeWireframe,
		&this->shadingModeShadedUntextured,
		&this->shadingModeUnshadedTextured,
		&this->shadingModeShadedTextured,
	};
	for (int i = 0; i < buttons.size(); i++) {
		auto* button = *buttons[i] = new QPushButton(this);
		button->setFixedSize(SHADING_MODE_BUTTON_SIZE, SHADING_MODE_BUTTON_SIZE);
		button->setFlat(true);
		button->setStyleSheet("QPushButton::pressed { background-color: rgba(0,0,0,0); border: none; }");
		QObject::connect(button, &QPushButton::pressed, [=, this] {
			this->setShadingMode(static_cast<MDLShadingMode>(i));
		});
		controlsLayout->addWidget(button, Qt::AlignRight);
	}

	this->mdl = new MDLWidget(this);
	layout->addWidget(this->mdl);
}

static std::optional<VTFData> getTextureDataForMaterial(const VPK& vpk, const std::string& materialPath) {
	auto materialEntry = vpk.findEntry(materialPath);
	if (!materialEntry) return std::nullopt;

	auto materialFile = vpk.readTextEntry(*materialEntry);
	if (!materialFile) return std::nullopt;

	KeyValueRoot materialKV;
	if (materialKV.Parse(materialFile->c_str()) != KeyValueErrorCode::NONE || !materialKV.HasChildren()) return std::nullopt;

	auto& baseTexturePathKV = materialKV.At(0).Get("$basetexture");
	if (!baseTexturePathKV.IsValid()) return std::nullopt;

	auto textureEntry = vpk.findEntry("materials/" + std::string{baseTexturePathKV.Value().string, baseTexturePathKV.Value().length} + ".vtf");
	if (!textureEntry) return std::nullopt;

	auto textureFile = vpk.readBinaryEntry(*textureEntry);
	if (!textureFile) return std::nullopt;

	VTFLib::CVTFFile vtf;
	vtf.Load(textureFile->data(), static_cast<vlUInt>(textureFile->size()), false);
	return VTFDecoder::decodeImage(vtf, 1, 1, 0, false);
}

void MDLPreview::setMesh(const QString& path, const VPK& vpk) const {
	this->mdl->clearMeshes();

	QString basePath = std::filesystem::path(path.toStdString()).replace_extension().string().c_str();
	if (path.endsWith(".vtx")) {
		// Remove .dx80, .dx90, .sw
		basePath = std::filesystem::path(basePath.toStdString()).replace_extension().string().c_str();
	}

	auto mdlEntry = vpk.findEntry(basePath.toStdString() + ".mdl");
	auto vvdEntry = vpk.findEntry(basePath.toStdString() + ".vvd");
	auto vtxEntry = vpk.findEntry(basePath.toStdString() + ".vtx");
	if (!vtxEntry) {
		vtxEntry = vpk.findEntry(basePath.toStdString() + ".dx90.vtx");
	}
	if (!vtxEntry) {
		vtxEntry = vpk.findEntry(basePath.toStdString() + ".dx80.vtx");
	}
	if (!vtxEntry) {
		vtxEntry = vpk.findEntry(basePath.toStdString() + ".sw.vtx");
	}
	if (!mdlEntry || !vvdEntry || !vtxEntry) {
		this->fileViewer->getPreview<InvalidMDLErrorPreview>()->setErrorMessage(tr("Unable to find all the required files the model is composed of!"));
		this->fileViewer->showPreview<InvalidMDLErrorPreview>();
		return;
	}
	auto mdlData = vpk.readBinaryEntry(*mdlEntry);
	auto vvdData = vpk.readBinaryEntry(*vvdEntry);
	auto vtxData = vpk.readBinaryEntry(*vtxEntry);
	if (!mdlData || !vvdData || !vtxData) {
		this->fileViewer->showPreview<FileLoadErrorPreview>();
		return;
	}

	StudioModel mdlParser;
	bool opened = mdlParser.open(reinterpret_cast<const uint8_t*>(mdlData->data()), mdlData->size(),
								 reinterpret_cast<const uint8_t*>(vtxData->data()), vtxData->size(),
								 reinterpret_cast<const uint8_t*>(vvdData->data()), vvdData->size());
    if (!opened) {
	    this->fileViewer->getPreview<InvalidMDLErrorPreview>()->setErrorMessage(tr("This model is invalid, it cannot be previewed!"));
	    this->fileViewer->showPreview<InvalidMDLErrorPreview>();
        return;
    }

	// Maybe we can add a setting for this...
	constexpr int currentLOD = ROOT_LOD;

	// According to my limited research, vertices stay constant (ignoring LOD fixups) but indices vary with LOD level
	// For our purposes we're also going to split the model up by material, don't know how Valve renders models
	QVector<MDLVertex> vertices;
	const auto convertVertexFormat = [](const VVD::Vertex& vertex) -> MDLVertex {
		return {
			QVector3D(vertex.position.x, vertex.position.y, vertex.position.z),
			QVector3D(vertex.normal.x, vertex.normal.y, vertex.normal.z),
			QVector2D(vertex.uv.x, vertex.uv.y),
		};
	};
	if (mdlParser.vvd.fixups.empty()) {
		for (const auto& vertex : mdlParser.vvd.vertices) {
			vertices.push_back(convertVertexFormat(vertex));
		}
	} else {
		for (const auto& fixup : mdlParser.vvd.fixups) {
			if (fixup.LOD < currentLOD) {
				continue;
			}
			std::span<VVD::Vertex> fixupVertices{mdlParser.vvd.vertices.begin() + fixup.sourceVertexID, static_cast<std::span<VVD::Vertex>::size_type>(fixup.vertexCount)};
			for (const auto& vertex : fixupVertices) {
				vertices.push_back(convertVertexFormat(vertex));
			}
		}
	}
	this->mdl->setVertices(vertices);

	this->mdl->setAABB({
		{mdlParser.mdl.hullMin.x, mdlParser.mdl.hullMin.y, mdlParser.mdl.hullMin.z},
		{mdlParser.mdl.hullMax.x, mdlParser.mdl.hullMax.y, mdlParser.mdl.hullMax.z},
	});

	bool hasAMaterial = false;
	std::size_t bodyPartTotalVertexOffset = 0;

	// todo: figure out why any set of indices past the first one is corrupted
	for (int bodyPartIndex = 0; bodyPartIndex < mdlParser.mdl.bodyParts.size(); bodyPartIndex++) {
		auto& mdlBodyPart = mdlParser.mdl.bodyParts.at(bodyPartIndex);
		auto& vtxBodyPart = mdlParser.vtx.bodyParts.at(bodyPartIndex);

		for (int modelIndex = 0; modelIndex < mdlBodyPart.models.size(); modelIndex++) {
			auto& mdlModel = mdlBodyPart.models.at(modelIndex);
			auto& vtxModel = vtxBodyPart.models.at(modelIndex);

			for (int meshIndex = 0; meshIndex < mdlModel.meshes.size(); meshIndex++) {
				auto& mdlMesh = mdlModel.meshes.at(meshIndex);
				auto materialIndex = mdlMesh.material;
				auto& vtxMesh = vtxModel.modelLODs.at(currentLOD).meshes.at(meshIndex);

				const std::size_t currentBodyPartTotalVertexOffset = bodyPartTotalVertexOffset;

				QVector<unsigned short> indices;

				for (const auto& stripGroup : vtxMesh.stripGroups) {
					bodyPartTotalVertexOffset += stripGroup.vertices.size();
					for (const auto& strip : stripGroup.strips) {
						const auto addIndex = [currentBodyPartTotalVertexOffset, &indices, &mdlModel, &stripGroup](int index) {
							indices.push_back(stripGroup.vertices[index].meshVertexID + mdlModel.verticesOffset + currentBodyPartTotalVertexOffset);
						};

						// Remember to flip the winding order
						if (strip.flags & VTX::Strip::FLAG_IS_TRILIST) {
							for (int i = 0; i < strip.indices.size(); i += 3) {
								addIndex(strip.indices[ i ]);
								addIndex(strip.indices[i+2]);
								addIndex(strip.indices[i+1]);
							}
						} else {
							for (auto i = strip.indices.size(); i >= 2; i -= 3) {
								addIndex(strip.indices[ i ]);
								addIndex(strip.indices[i-2]);
								addIndex(strip.indices[i-1]);
							}
						}
					}
				}

				if (mdlParser.mdl.materials.size() <= materialIndex) {
					this->mdl->addSubMesh(indices);
					continue;
				}
				// Try to find the material in the VPK
				bool foundMaterial = false;
				for (int materialDirIndex = 0; materialDirIndex < mdlParser.mdl.materialDirectories.size(); materialDirIndex++) {
					if (auto data = getTextureDataForMaterial(vpk, "materials/"s + mdlParser.mdl.materialDirectories.at(materialDirIndex) + mdlParser.mdl.materials.at(materialIndex).name + ".vmt")) {
						this->mdl->addSubMesh(indices, std::move(data.value()));
						foundMaterial = true;
						hasAMaterial = true;
						break;
					}
				}
				if (!foundMaterial) {
					this->mdl->addSubMesh(indices);
				}
			}
		}
	}

	if (hasAMaterial) {
		this->setShadingMode(MDLShadingMode::UNSHADED_TEXTURED);
	} else {
		this->setShadingMode(MDLShadingMode::SHADED_UNTEXTURED);
	}
	this->mdl->update();
}

void MDLPreview::setShadingMode(MDLShadingMode mode) const {
	QStyleOption opt;
	opt.initFrom(this);

	this->backfaceCulling->setDisabled(mode == MDLShadingMode::WIREFRAME);

	const QList<std::tuple<QPushButton* const*, QString, MDLShadingMode>> buttonsAndIcons{
			{&this->shadingModeWireframe, ":/icons/wireframe.png", MDLShadingMode::WIREFRAME},
			{&this->shadingModeShadedUntextured, ":/icons/shaded_untextured.png", MDLShadingMode::SHADED_UNTEXTURED},
			{&this->shadingModeUnshadedTextured, ":/icons/unshaded_textured.png", MDLShadingMode::UNSHADED_TEXTURED},
			{&this->shadingModeShadedTextured, ":/icons/shaded_textured.png", MDLShadingMode::SHADED_TEXTURED},
	};
	for (auto& [button, iconPath, buttonMode] : buttonsAndIcons) {
		QPixmap imagePixmap(iconPath);
		auto mask = imagePixmap.createMaskFromColor(Qt::white, Qt::MaskOutColor);
		imagePixmap.fill(opt.palette.color(buttonMode == mode ? QPalette::ColorRole::Link : QPalette::ColorRole::ButtonText));
		imagePixmap.setMask(mask);

		(*button)->setIcon({imagePixmap});
		(*button)->setIconSize({SHADING_MODE_BUTTON_SIZE, SHADING_MODE_BUTTON_SIZE});
	}

	this->mdl->setShadingMode(mode);
}
