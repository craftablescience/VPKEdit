#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <QBasicTimer>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

#include "../IVPKEditPreviewPlugin_V1_0.h"

namespace mdlpp {

struct BakedModel;

} // namespace mdlpp

class QCheckBox;
class QKeyEvent;
class QMouseEvent;
class QSpinBox;
class QTabWidget;
class QTimerEvent;
class QToolButton;
class QTreeWidget;

struct AABB {
	[[nodiscard]] QList<QVector3D> getCorners() const;

	[[nodiscard]] float getWidth() const;

	[[nodiscard]] float getHeight() const;

	[[nodiscard]] float getDepth() const;

	QVector3D min;
	QVector3D max;
};

struct MDLSubMesh {
	std::unique_ptr<QOpenGLVertexArrayObject> vao;
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

struct MDLTextureSettings {
	enum class TransparencyMode {
		NONE,
		ALPHA_TEST,
		TRANSLUCENT,
	} transparencyMode = TransparencyMode::NONE;
	float alphaTestReference = 0.7f;
};

struct MDLTextureData {
	std::vector<std::byte> data;
	std::uint16_t width;
	std::uint16_t height;
	MDLTextureSettings settings;
};

class MDLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
	Q_OBJECT;

public:
	explicit MDLWidget(QWidget* parent = nullptr);

	~MDLWidget() override;

	void setModel(const mdlpp::BakedModel& model);

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
	std::vector<MDLSubMesh> meshes;
	QList<QPair<QOpenGLTexture*, MDLTextureSettings>> textures;

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

class MDLPreview final : public IVPKEditPreviewPlugin_V1_0 {
	Q_OBJECT;
	Q_PLUGIN_METADATA(IID IVPKEditPreviewPlugin_V1_0_iid FILE "MDLPreview.json");
	Q_INTERFACES(IVPKEditPreviewPlugin_V1_0);

public:
	void initPlugin(IVPKEditPreviewPlugin_V1_0_IWindowAccess* windowAccess_) override;

	void initPreview(QWidget* parent) override;

	[[nodiscard]] QWidget* getPreview() const override;

	[[nodiscard]] QIcon getIcon() const override;

	Error setData(const QString& path, const quint8* dataPtr, quint64 length) override;

private:
	void setShadingMode(MDLShadingMode mode) const;

	IVPKEditPreviewPlugin_V1_0_IWindowAccess* windowAccess = nullptr;
	QWidget* preview = nullptr;

	QCheckBox* backfaceCulling = nullptr;
	QSpinBox* skinSpinBox = nullptr;
	QToolButton* shadingModeWireframe = nullptr;
	QToolButton* shadingModeShadedUntextured = nullptr;
	QToolButton* shadingModeUnshadedTextured = nullptr;
	QToolButton* shadingModeShadedTextured = nullptr;

	MDLWidget* mdl = nullptr;

	QTabWidget* tabs = nullptr;
	QTreeWidget* materialsTab = nullptr;
	QTreeWidget* allMaterialsTab = nullptr;
};
