#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

namespace locus::gui {

class Locus3DViewport : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit Locus3DViewport(QWidget* parent = nullptr);
    ~Locus3DViewport() override;

protected:  Que isso?
    // Métodos essenciais do ciclo de vida de renderização do Qt
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;
};

} // namespace locus::gui