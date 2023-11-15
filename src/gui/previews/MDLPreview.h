#pragma once

#include <memory>
#include <vector>

#include <QBasicTimer>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

#include "../formats/VTFDecoder.h"

namespace vpkedit {

class VPK;

} // namespace vpkedit

class QMouseEvent;
class QTimerEvent;

#pragma pack(push, 1)
struct MDLVertex {
	MDLVertex(QVector3D pos_, QVector3D normal_, QVector2D uv_)
			: pos(pos_)
			, normal(normal_)
			, uv(uv_) {}

	QVector3D pos;
	QVector3D normal;
	QVector2D uv;
};
#pragma pack(pop)

struct AABB {
	[[nodiscard]] QList<QVector3D> getCorners() const;

	[[nodiscard]] float getWidth() const;

	[[nodiscard]] float getHeight() const;

	[[nodiscard]] float getDepth() const;

	QVector3D min;
	QVector3D max;
};

struct MDLSubMesh {
	VTFData vtfData;
	QOpenGLTexture* texture;
	QOpenGLBuffer ebo{QOpenGLBuffer::Type::IndexBuffer};
	int indexCount;
};

enum class MDLShadingType {
	SHADED_UNTEXTURED,
	UNSHADED_TEXTURED,
	SHADED_TEXTURED,
};

class MDLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
	Q_OBJECT;

public:
	explicit MDLWidget(QWidget *parent = nullptr);

	~MDLWidget() override;

	void setVertices(const QVector<MDLVertex>& vertices_);

	void addSubMesh(const QVector<unsigned short>& indices);

	void addSubMesh(const QVector<unsigned short>& indices, VTFData&& vtfData);

	void setAABB(AABB aabb);

	[[nodiscard]] MDLShadingType getShadingType() const { return this->shadingType; }

	void setShadingType(MDLShadingType type);

	void clearMeshes();

protected:
	void initializeGL() override;

	void resizeGL(int w, int h) override;

	void paintGL() override;

	void mousePressEvent(QMouseEvent* event) override;

	void mouseReleaseEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

	void timerEvent(QTimerEvent* event) override;

private:
	QOpenGLShaderProgram shadedUntexturedShaderProgram;
	QOpenGLShaderProgram unshadedTexturedShaderProgram;
	QOpenGLShaderProgram shadedTexturedShaderProgram;
	QOpenGLTexture missingTexture;
	QOpenGLBuffer vertices{QOpenGLBuffer::Type::VertexBuffer};
	int vertexCount;
	std::vector<MDLSubMesh> meshes;

	MDLShadingType shadingType;
	QMatrix4x4 projection;
	float distance;
	QVector3D target;
	float fov;

	QBasicTimer timer;
	QVector2D mousePressPosition;
	QVector3D rotationAxis;
	QVector3D translationalVelocity;
	qreal angularSpeed;
	QQuaternion rotation;
	bool rmbBeingHeld;
};

class MDLPreview : public QWidget {
	Q_OBJECT;

public:
	static inline const QStringList EXTENSIONS {
			".mdl",
			".vtx",
			".vvd",
			".phy",
			".ani",
	};

	explicit MDLPreview(QWidget* parent = nullptr);

	void setMesh(const QString& path, const vpkedit::VPK& vpk) const;

private:
	MDLWidget* mdl;
};
