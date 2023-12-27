#pragma once

#include <memory>
#include <vector>

#include <QBasicTimer>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

#include "../formats/VTFDecoder.h"

namespace vpkedit {

class VPK;

} // namespace vpkedit

class QCheckBox;
class QMouseEvent;
class QTimerEvent;
class QToolButton;

class FileViewer;

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

enum class MDLShadingMode {
	WIREFRAME = 0,
	SHADED_UNTEXTURED = 1,
	UNSHADED_TEXTURED = 2,
	SHADED_TEXTURED = 3,
};

class MDLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_2_Core {
	Q_OBJECT;

public:
	explicit MDLWidget(QWidget *parent = nullptr);

	~MDLWidget() override;

	void setVertices(const QList<MDLVertex>& vertices_);

	void addSubMesh(const QList<unsigned short>& indices);

	void addSubMesh(const QList<unsigned short>& indices, VTFData&& vtfData);

	void setAABB(AABB aabb);

	[[nodiscard]] MDLShadingMode getShadingMode() const { return this->shadingMode; }

	void setShadingMode(MDLShadingMode type);

	[[nodiscard]] float getFieldOfView() const { return this->fov; };

	void setFieldOfView(float newFOV);

	[[nodiscard]] bool isCullBackFaces() const { return this->cullBackFaces; };

	void setCullBackFaces(bool enable);

	void clearMeshes();

protected:
	void initializeGL() override;

	void resizeGL(int w, int h) override;

	void paintGL() override;

	void mousePressEvent(QMouseEvent* event) override;

	void mouseReleaseEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

	void wheelEvent(QWheelEvent* event) override;

	void timerEvent(QTimerEvent* event) override;

private:
	QOpenGLShaderProgram wireframeShaderProgram;
	QOpenGLShaderProgram shadedUntexturedShaderProgram;
	QOpenGLShaderProgram unshadedTexturedShaderProgram;
	QOpenGLShaderProgram shadedTexturedShaderProgram;
	QOpenGLTexture missingTexture;
	QOpenGLTexture matCapTexture;
	QOpenGLBuffer vertices{QOpenGLBuffer::Type::VertexBuffer};
	int vertexCount;
	std::vector<MDLSubMesh> meshes;

	MDLShadingMode shadingMode;
	QMatrix4x4 projection;
	float distance;
    float distanceScale;
	QVector3D target;
	float fov;
	bool cullBackFaces;

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
			".vta",
	};

	explicit MDLPreview(FileViewer* fileViewer_, QWidget* parent = nullptr);

	void setMesh(const QString& path, const vpkedit::VPK& vpk) const;

private:
	void setShadingMode(MDLShadingMode mode) const;

	FileViewer* fileViewer;

	MDLWidget* mdl;
	QCheckBox* backfaceCulling;
    QToolButton* shadingModeWireframe;
    QToolButton* shadingModeShadedUntextured;
    QToolButton* shadingModeUnshadedTextured;
    QToolButton* shadingModeShadedTextured;
};
