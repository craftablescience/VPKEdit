#pragma once

#include <QBasicTimer>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

namespace vpkedit {

class VPK;

} // namespace vpkedit

class QMouseEvent;
class QTimerEvent;

#pragma pack(push, 1)
struct MDLVertex {
	MDLVertex(QVector3D pos_, QVector2D uv_)
			: pos(pos_)
			, uv(uv_) {}

	QVector3D pos;
	QVector2D uv;
};
#pragma pack(pop)

struct AABB {
	QVector3D min;
	QVector3D max;
};

struct MDLSubMesh {
	QOpenGLBuffer vbo{QOpenGLBuffer::Type::VertexBuffer};
	QOpenGLBuffer ebo{QOpenGLBuffer::Type::IndexBuffer};
	int vertexCount;
	int indexCount;
};

class MDLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
	Q_OBJECT;

public:
	explicit MDLWidget(QWidget *parent = nullptr);

	~MDLWidget() override;

	void addMesh(const QVector<MDLVertex>& vertices, const QVector<unsigned short>& indices);

	void setAABB(AABB aabb);

	void clearMeshes();

protected:
	void initializeGL() override;

	void resizeGL(int w, int h) override;

	void paintGL() override;

	void mousePressEvent(QMouseEvent* event) override;

	void mouseReleaseEvent(QMouseEvent* event) override;

	void timerEvent(QTimerEvent* event) override;

private:
	QOpenGLShaderProgram shaderProgram;
	QOpenGLTexture modelTexture;
	QVector<MDLSubMesh> meshes;

	QMatrix4x4 projection;
	float distance;
	QVector3D target;
	float fov;

	QBasicTimer timer;
	QVector2D mousePressPosition;
	QVector3D rotationAxis;
	qreal angularSpeed;
	QQuaternion rotation;
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
