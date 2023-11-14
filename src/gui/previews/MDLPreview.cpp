#include "MDLPreview.h"

// temp fix for mdlparser
#include <stddef.h>
#include <stdint.h>

#include <filesystem>

#include <MDLParser.h>
#include <QMouseEvent>
#include <QStyleOption>
#include <QVBoxLayout>
#include <QtMath>
#include <vpkedit/VPK.h>

using namespace vpkedit;

MDLWidget::MDLWidget(QWidget* parent)
		: QOpenGLWidget(parent)
		, modelTexture(QOpenGLTexture::Target2D)
		, distance(0.0)
		, fov(70.0)
		, angularSpeed(0.0) {}

MDLWidget::~MDLWidget() {
	this->clearMeshes();
}

void MDLWidget::addMesh(const QVector<MDLVertex>& vertices, const QVector<unsigned short>& indices) {
	// todo: load the texture from the vtf if it exists
	if (!this->modelTexture.isCreated()) {
		this->modelTexture.setData(QImage(":/checkerboard.png"));
	}

	auto& mesh = this->meshes.emplace_back();

	mesh.vertexCount = static_cast<int>(vertices.size());
	mesh.vbo.create();
	mesh.vbo.bind();
	mesh.vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
	mesh.vbo.allocate(vertices.constData(), static_cast<int>(mesh.vertexCount * sizeof(MDLVertex)));
	mesh.vbo.release();

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

void MDLWidget::clearMeshes() {
	if (this->modelTexture.isCreated()) {
		this->modelTexture.destroy();
	}

	for (auto& mesh : this->meshes) {
		if (mesh.vbo.isCreated()) {
			mesh.vbo.destroy();
		}
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

	this->shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

uniform mat4 mvp;

attribute vec4 iPos;
attribute vec2 iUV;

varying vec2 oUV;

void main() {
    gl_Position = mvp * iPos;
    oUV = iUV;
}
	)");
	this->shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

uniform sampler2D texture;

varying vec2 oUV;

void main() {
    gl_FragColor = texture2D(texture, oUV);
}
	)");
	this->shaderProgram.link();

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
	//this->glEnable(GL_CULL_FACE);

	this->shaderProgram.bind();

	QMatrix4x4 view;
	view.translate(this->target.x(), this->target.y(), -this->target.z() - this->distance);
	view.rotate(this->rotation);
	this->shaderProgram.setUniformValue("mvp", this->projection * view);
	this->shaderProgram.setUniformValue("texture", 0);

	this->modelTexture.bind();

	for (auto& mesh : this->meshes) {
		mesh.vbo.bind();
		mesh.ebo.bind();
		int offset = 0;

		int vertexPosLocation = this->shaderProgram.attributeLocation("iPos");
		this->shaderProgram.enableAttributeArray(vertexPosLocation);
		this->shaderProgram.setAttributeBuffer(vertexPosLocation, GL_FLOAT, offset, 3, sizeof(MDLVertex));

		offset += sizeof(QVector3D);
		int vertexUVLocation = this->shaderProgram.attributeLocation("iUV");
		this->shaderProgram.enableAttributeArray(vertexUVLocation);
		this->shaderProgram.setAttributeBuffer(vertexUVLocation, GL_FLOAT, offset, 2, sizeof(MDLVertex));

		this->glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_SHORT, nullptr);
		mesh.ebo.release();
		mesh.vbo.release();
	}

	this->modelTexture.release();

	this->shaderProgram.release();
}

void MDLWidget::mousePressEvent(QMouseEvent* event) {
	this->mousePressPosition = QVector2D(event->position());
}

void MDLWidget::mouseReleaseEvent(QMouseEvent* event) {
	// Mouse release position - mouse press position
	QVector2D diff = QVector2D(event->position()) - this->mousePressPosition;

	// Rotation axis is perpendicular to the mouse position difference vector
	QVector3D n = QVector3D(diff.y(), diff.x(), 0.0).normalized();

	// Accelerate angular speed relative to the length of the mouse sweep
	float acc = diff.length() / 100.0f;

	// Calculate new rotation axis as weighted sum
	this->rotationAxis = (this->rotationAxis * static_cast<float>(this->angularSpeed) + n * acc).normalized();

	// Increase angular speed
	this->angularSpeed += acc;
}

void MDLWidget::timerEvent(QTimerEvent* /*event*/) {
	this->angularSpeed *= 0.99;
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

void MDLPreview::setMesh(const QString& path, const VPK& vpk) const {
	this->mdl->clearMeshes();

	const QString basePath = std::filesystem::path(path.toStdString()).replace_extension().string().c_str();

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
		// todo: show an error preview?
		return;
	}
	auto mdlData = vpk.readBinaryEntry(*mdlEntry);
	auto vvdData = vpk.readBinaryEntry(*vvdEntry);
	auto vtxData = vpk.readBinaryEntry(*vtxEntry);
	if (!mdlData || !vvdData || !vtxData) {
		// todo: show an error preview?
		return;
	}

	MDL mdlParser(reinterpret_cast<const uint8_t*>(mdlData->data()), mdlData->size(),
	              reinterpret_cast<const uint8_t*>(vvdData->data()), vvdData->size(),
	              reinterpret_cast<const uint8_t*>(vtxData->data()), vtxData->size());
	QVector3D minAABB, maxAABB;

	QVector<MDLVertex> vertices;
	// According to my limited research, vertices stay constant but indices vary with LOD level
	// We're just loading the highest detail model for now, so it doesn't matter
	QVector<unsigned short> indices;

	// todo: apply lod fixups?
	for (int vertexIndex = 0; vertexIndex < mdlParser.GetNumVertices(); vertexIndex++) {
		auto* vertex = mdlParser.GetVertex(vertexIndex);

		QVector3D pos(vertex->pos.x, vertex->pos.y, vertex->pos.z);
		QVector2D uv(vertex->texCoord.x, vertex->texCoord.y);
		vertices.emplace_back(pos, uv);

		if (vertex->pos.x < minAABB.x()) minAABB.setX(vertex->pos.x);
		else if (vertex->pos.x > maxAABB.x()) maxAABB.setX(vertex->pos.x);
		if (vertex->pos.y < minAABB.y()) minAABB.setY(vertex->pos.y);
		else if (vertex->pos.y > maxAABB.y()) maxAABB.setY(vertex->pos.y);
		if (vertex->pos.z < minAABB.z()) minAABB.setZ(vertex->pos.z);
		else if (vertex->pos.z > maxAABB.z()) maxAABB.setZ(vertex->pos.z);
	}

	for (int body = 0; body < mdlParser.GetNumBodyParts(); body++) {
		const MDLStructs::BodyPart* mdlBodyPart;
		const VTXStructs::BodyPart* vtxBodyPart;
		mdlParser.GetBodyPart(body, &mdlBodyPart, &vtxBodyPart);

		for (int modelIndex = 0; modelIndex < vtxBodyPart->numModels; modelIndex++) {
			auto* modelMDL = mdlBodyPart->GetModel(modelIndex);
			auto* modelVTX = vtxBodyPart->GetModel(modelIndex);

			for (int meshIndex = 0; meshIndex < modelMDL->meshesCount; meshIndex++) {
				auto* modelLOD = modelVTX->GetModelLoD(0);
				auto* mesh = modelLOD->GetMesh(meshIndex);

				for (int stripGroupIndex = 0; stripGroupIndex < mesh->numStripGroups; stripGroupIndex++) {
					auto* stripGroup = mesh->GetStripGroup(stripGroupIndex);

					for (int stripIndex = 0; stripIndex < stripGroup->numStrips; stripIndex++) {
						auto* strip = stripGroup->GetStrip(stripIndex);

						bool isTriList = static_cast<unsigned char>(strip->flags) & static_cast<unsigned char>(VTXEnums::StripFlags::IS_TRILIST);

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
			}
		}
	}

	for (unsigned short i : indices) {
		qInfo() << i;
	}

	this->mdl->addMesh(vertices, indices);
	this->mdl->setAABB({minAABB, maxAABB});
}
