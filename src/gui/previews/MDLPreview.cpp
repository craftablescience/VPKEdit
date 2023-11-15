#include "MDLPreview.h"

#include <filesystem>
#include <optional>

#include <MDLParser.h>
#include <QMouseEvent>
#include <QStyleOption>
#include <QVBoxLayout>
#include <QtMath>
#include <vpkedit/VPK.h>
#include <VTFLib.h>

// Fuck windows
#undef NO_ERROR
#include "KeyValue.h"

using namespace std::literals;
using namespace vpkedit;

MDLWidget::MDLWidget(QWidget* parent)
		: QOpenGLWidget(parent)
		, missingTexture(QOpenGLTexture::Target2D)
		, vertexCount(0)
		, shadingType(MDLShadingType::SHADED_UNTEXTURED)
		, distance(0.0)
		, fov(70.0)
		, angularSpeed(0.0) {}

MDLWidget::~MDLWidget() {
	this->clearMeshes();
	this->missingTexture.destroy();
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
	float distanceToMin = midpoint.distanceToPoint(aabb.min);
	float distanceToMax = midpoint.distanceToPoint(aabb.max);
	float sphereRadius;
	if (distanceToMax > distanceToMin) {
		sphereRadius = distanceToMax;
	} else {
		sphereRadius = distanceToMin;
	}
	float fovRad = qDegreesToRadians(this->fov);
	this->target = midpoint;
	this->distance = static_cast<float>(sphereRadius / qTan(fovRad / 2));

	this->update();
}

void MDLWidget::setShadingType(MDLShadingType type) {
	this->shadingType = type;
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
	this->initializeOpenGLFunctions();

	QStyleOption opt;
	opt.initFrom(this);
	auto clearColor = opt.palette.color(QPalette::ColorRole::Window);
	this->glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF());

	this->shadedUntexturedShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shaded_untextured.vert");
	this->shadedUntexturedShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shaded_untextured.frag");
	this->shadedUntexturedShaderProgram.link();

	this->unlitTexturedShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/unlit_textured.vert");
	this->unlitTexturedShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/unlit_textured.frag");
	this->unlitTexturedShaderProgram.link();

	this->shadedTexturedShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shaded_textured.vert");
	this->shadedTexturedShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shaded_textured.frag");
	this->shadedTexturedShaderProgram.link();

	this->missingTexture.create();
	this->missingTexture.setData(QImage(":/checkerboard.png"));

	this->timer.start(16, this);
}

void MDLWidget::resizeGL(int w, int h) {
	this->glViewport(0, 0, w, h);

	const float aspectRatio = static_cast<float>(w) / static_cast<float>(h > 0 ? h : 1);
	const float nearPlane = 0.025f, farPlane = 1024.0f;
	this->projection.setToIdentity();
	this->projection.perspective(this->fov, aspectRatio, nearPlane, farPlane);
}

void MDLWidget::paintGL() {
	this->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (this->meshes.empty()) {
		return;
	}

	this->glEnable(GL_MULTISAMPLE);
	this->glEnable(GL_DEPTH_TEST);
	this->glEnable(GL_CULL_FACE);

	QOpenGLShaderProgram* currentShaderProgram = nullptr;
	switch (this->shadingType) {
		case MDLShadingType::SHADED_UNTEXTURED:
			currentShaderProgram = &this->shadedUntexturedShaderProgram;
			break;
		case MDLShadingType::UNLIT_TEXTURED:
			currentShaderProgram = &this->unlitTexturedShaderProgram;
			break;
		case MDLShadingType::SHADED_TEXTURED:
			currentShaderProgram = &this->shadedTexturedShaderProgram;
			break;
	}
	if (!currentShaderProgram) {
		return;
	}
	currentShaderProgram->bind();

	QMatrix4x4 view;
	view.translate(this->target.x(), this->target.y(), -this->target.z() - this->distance);
	view.rotate(this->rotation);
	currentShaderProgram->setUniformValue("mvp", this->projection * view);
	currentShaderProgram->setUniformValue("texture", 0);

	this->vertices.bind();

	for (auto& mesh : this->meshes) {
		auto* texture = mesh.texture;
		if (!texture) {
			texture = &this->missingTexture;
		}
		texture->bind();

		mesh.ebo.bind();
		int offset = 0;

		int vertexPosLocation = currentShaderProgram->attributeLocation("iPos");
		currentShaderProgram->enableAttributeArray(vertexPosLocation);
		currentShaderProgram->setAttributeBuffer(vertexPosLocation, GL_FLOAT, offset, 3, sizeof(MDLVertex));

		offset += sizeof(QVector3D);
		int vertexNormalLocation = currentShaderProgram->attributeLocation("iNormal");
		currentShaderProgram->enableAttributeArray(vertexNormalLocation);
		currentShaderProgram->setAttributeBuffer(vertexNormalLocation, GL_FLOAT, offset, 3, sizeof(MDLVertex));

		offset += sizeof(QVector3D);
		int vertexUVLocation = currentShaderProgram->attributeLocation("iUV");
		currentShaderProgram->enableAttributeArray(vertexUVLocation);
		currentShaderProgram->setAttributeBuffer(vertexUVLocation, GL_FLOAT, offset, 2, sizeof(MDLVertex));

		this->glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_SHORT, nullptr);
		mesh.ebo.release();

		texture->release();
	}

	this->vertices.release();

	currentShaderProgram->release();
}

void MDLWidget::mousePressEvent(QMouseEvent* event) {
	this->mousePressPosition = QVector2D(event->position());
}

void MDLWidget::mouseMoveEvent(QMouseEvent* event) {
	QVector2D diff = QVector2D(event->position()) - this->mousePressPosition;

	// Rotation axis is perpendicular to the mouse position difference vector
	QVector3D inputAxis = QVector3D(diff.y(), diff.x(), 0.0).normalized();

	// Accelerate relative to the length of the mouse sweep
	float acceleration = diff.length();

    // Update rotation axis, velocity
	this->rotationAxis = (acceleration * inputAxis).normalized();
	this->angularSpeed = acceleration;

    // Update old position
	this->mousePressPosition = QVector2D(event->position());
}

void MDLWidget::timerEvent(QTimerEvent* /*event*/) {
	this->angularSpeed *= 0.75;
	if (this->angularSpeed < 0.01) {
		this->angularSpeed = 0.0;
	} else {
		this->rotation = QQuaternion::fromAxisAndAngle(this->rotationAxis, static_cast<float>(this->angularSpeed)) * this->rotation;
		this->update();
	}
}

MDLPreview::MDLPreview(QWidget* parent)
		: QWidget(parent) {
	auto* layout = new QVBoxLayout(this);

	this->mdl = new MDLWidget(this);
	layout->addWidget(this->mdl);
}

static std::optional<VTFData> getTextureDataForMaterial(const VPK& vpk, const std::string& materialPath) {
	auto materialEntry = vpk.findEntry(materialPath);
	if (!materialEntry) return std::nullopt;

	auto materialFile = vpk.readTextEntry(*materialEntry);
	if (!materialFile) return std::nullopt;

	KeyValueRoot materialKV;
	if (materialKV.Parse(materialFile->c_str()) != KeyValueErrorCode::NO_ERROR || !materialKV.HasChildren()) return std::nullopt;

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
		// todo: show an error
		return;
	}
	auto mdlData = vpk.readBinaryEntry(*mdlEntry);
	auto vvdData = vpk.readBinaryEntry(*vvdEntry);
	auto vtxData = vpk.readBinaryEntry(*vtxEntry);
	if (!mdlData || !vvdData || !vtxData) {
		// todo: show an error
		return;
	}

	MDL mdlParser(reinterpret_cast<const uint8_t*>(mdlData->data()), mdlData->size(),
	              reinterpret_cast<const uint8_t*>(vvdData->data()), vvdData->size(),
	              reinterpret_cast<const uint8_t*>(vtxData->data()), vtxData->size());
    if (!mdlParser.IsValid()) {
        // todo: show an error
        return;
    }

    AABB aabb;

	// According to my limited research, vertices stay constant (ignoring LOD fixups) but indices vary with LOD level
	// For our purposes we're also going to split the model up by material, don't know how Valve renders models
	QVector<MDLVertex> vertices;

	// todo: apply lod fixup if it exists
	for (int vertexIndex = 0; vertexIndex < mdlParser.GetNumVertices(); vertexIndex++) {
		auto* vertex = mdlParser.GetVertex(vertexIndex);

		QVector3D pos(vertex->pos.x, vertex->pos.y, vertex->pos.z);
		QVector3D normal(vertex->normal.x, vertex->normal.y, vertex->normal.z);
		QVector2D uv(vertex->texCoord.x, vertex->texCoord.y);
		vertices.emplace_back(pos, normal, uv);

		// Resize bounding box
		if (vertex->pos.x < aabb.min.x()) aabb.min.setX(vertex->pos.x);
		else if (vertex->pos.x > aabb.max.x()) aabb.max.setX(vertex->pos.x);
		if (vertex->pos.y < aabb.min.y()) aabb.min.setY(vertex->pos.y);
		else if (vertex->pos.y > aabb.max.y()) aabb.max.setY(vertex->pos.y);
		if (vertex->pos.z < aabb.min.z()) aabb.min.setZ(vertex->pos.z);
		else if (vertex->pos.z > aabb.max.z()) aabb.max.setZ(vertex->pos.z);
	}

	this->mdl->setVertices(vertices);
	this->mdl->setAABB(aabb);

	for (int body = 0; body < mdlParser.GetNumBodyParts(); body++) {
		const MDLStructs::BodyPart* mdlBodyPart;
		const VTXStructs::BodyPart* vtxBodyPart;
		mdlParser.GetBodyPart(body, &mdlBodyPart, &vtxBodyPart);

		for (int modelIndex = 0; modelIndex < vtxBodyPart->numModels; modelIndex++) {
			auto* modelMDL = mdlBodyPart->GetModel(modelIndex);
			auto* modelVTX = vtxBodyPart->GetModel(modelIndex);

			for (int meshIndex = 0; meshIndex < modelMDL->meshesCount; meshIndex++) {
				auto* meshMDL = modelMDL->GetMesh(meshIndex);
				auto materialIndex = meshMDL->material;
				auto* meshVTX = modelVTX->GetModelLoD(0)->GetMesh(meshIndex);

				QVector<unsigned short> indices;

				for (int stripGroupIndex = 0; stripGroupIndex < meshVTX->numStripGroups; stripGroupIndex++) {
					auto* stripGroup = meshVTX->GetStripGroup(stripGroupIndex);

					for (int stripIndex = 0; stripIndex < stripGroup->numStrips; stripIndex++) {
						auto* strip = stripGroup->GetStrip(stripIndex);

						bool isTriList = static_cast<unsigned char>(strip->flags) & static_cast<unsigned char>(VTXEnums::StripFlags::IS_TRILIST);

						// Add vertices in reverse order to flip the winding order
						if (isTriList) {
							for (int i = strip->numIndices - 1; i >= 0; i--) {
								auto vertexID = *stripGroup->GetIndex(strip->indexOffset + i);
								auto vertex = stripGroup->GetVertex(vertexID);
								indices.push_back(vertex->origMeshVertId + modelMDL->vertsOffset);
							}
						} else {
							for (int i = strip->numIndices + strip->indexOffset; i >= strip->indexOffset + 2; i--) {
								indices.push_back(stripGroup->GetVertex(*stripGroup->GetIndex(  i  ))->origMeshVertId + modelMDL->vertsOffset);
								indices.push_back(stripGroup->GetVertex(*stripGroup->GetIndex(i - 2))->origMeshVertId + modelMDL->vertsOffset);
								indices.push_back(stripGroup->GetVertex(*stripGroup->GetIndex(i - 1))->origMeshVertId + modelMDL->vertsOffset);
							}
						}
					}
				}

				if (mdlParser.GetNumMaterials() < materialIndex) {
					this->mdl->addSubMesh(indices);
					continue;
				}
				// Try to find the material in the VPK
				bool foundMaterial = false;
				for (int materialDirIndex = 0; materialDirIndex < mdlParser.GetNumMaterialDirectories(); materialDirIndex++) {
					if (auto data = getTextureDataForMaterial(vpk, "materials/"s + mdlParser.GetMaterialDirectory(materialIndex) + mdlParser.GetMaterialName(materialIndex) + ".vmt")) {
						this->mdl->addSubMesh(indices, std::move(data.value()));
						foundMaterial = true;
						break;
					}
				}
				if (!foundMaterial) {
					this->mdl->addSubMesh(indices);
				}
			}
		}
	}

	this->mdl->update();
}
