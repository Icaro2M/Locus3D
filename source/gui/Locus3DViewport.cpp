#include "Locus3DViewport.h"

namespace locus::gui {

Locus3DViewport::Locus3DViewport(QWidget* parent)
    : QOpenGLWidget(parent) {
    // Permite foco de teclado para captura de atalhos e navegação da câmera[cite: 1]
    setFocusPolicy(Qt::StrongFocus);
}

Locus3DViewport::~Locus3DViewport() {
    // Limpeza obrigatória do contexto OpenGL antes da destruição do widget[cite: 1]
    makeCurrent();
    doneCurrent();
}

void Locus3DViewport::initializeGL() {
    initializeOpenGLFunctions();
    
    // Fundo cinza escuro profissional padrão de modeladores 3D (RGBA)[cite: 1]
    glClearColor(0.18f, 0.18f, 0.18f, 1.0f);
    
    // Habilita teste de profundidade para rasterização sólida[cite: 1]
    glEnable(GL_DEPTH_TEST);
}

void Locus3DViewport::resizeGL(int width, int height) {
    // Prevenção de travamento matemático por divisão por zero[cite: 1]
    if (height == 0) height = 1;
    glViewport(0, 0, width, height);
}

void Locus3DViewport::paintGL() {
    // Limpa os buffers a cada ciclo (frame) para desenhar o novo estado da cena[cite: 1]
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

} // namespace locus::gui