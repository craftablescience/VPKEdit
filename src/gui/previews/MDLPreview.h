#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <QBasicTimer>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

namespace vpkedit {

class PackFile;

} // namespace vpkedit

class QCheckBox;
class QKeyEvent;
class QMouseEvent;
class QSpinBox;
class QTabWidget;
class QTimerEvent;
class QToolButton;
class QTreeWidget;

class FileViewer;
class Window;

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
	int textureIndex;
	QOpenGLBuffer ebo{QOpenGLBuffer::Type::IndexBuffer};
	int indexCount;
};

enum class MDLShadingMode {
	WIREFRAME = 0,
	SHADED_UNTEXTURED = 1,
	UNSHADED_TEXTURED = 2,
	SHADED_TEXTURED = 3,
};

struct MDLTextureData {
	std::vector<std::byte> data;
	std::uint16_t width;
	std::uint16_t height;
};

class MDLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_2_Core {
	Q_OBJECT;

public:
	explicit MDLWidget(QWidget* parent = nullptr);

	~MDLWidget() override;

	void setVertices(const QList<MDLVertex>& vertices_);

	void addSubMesh(const QList<unsigned short>& indices, int textureIndex);

	void setTextures(const std::vector<std::unique_ptr<MDLTextureData>>& vtfData);

	void clearTextures();

	void setSkinLookupTable(std::vector<std::vector<short>> skins_);

	void setAABB(AABB aabb);

	[[nodiscard]] int getSkin() const { return this->skin; }

	void setSkin(int skin_);

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
	QList<MDLSubMesh> meshes;
	QList<QOpenGLTexture*> textures;

	int skin;
	std::vector<std::vector<short>> skins;

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

	explicit MDLPreview(FileViewer* fileViewer_, Window* window, QWidget* parent = nullptr);

	void setMesh(const QString& path, const vpkedit::PackFile& packFile) const;

private:
	void setShadingMode(MDLShadingMode mode) const;

	FileViewer* fileViewer;

	QCheckBox* backfaceCulling;
	QSpinBox* skinSpinBox;
	QToolButton* shadingModeWireframe;
	QToolButton* shadingModeShadedUntextured;
	QToolButton* shadingModeUnshadedTextured;
	QToolButton* shadingModeShadedTextured;

	MDLWidget* mdl;

	QTabWidget* tabs;
	QTreeWidget* materialsTab;
	QTreeWidget* allMaterialsTab;
};
