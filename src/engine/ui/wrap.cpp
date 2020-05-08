#include "wrap.h"
UI::UI(Platform::Application* app_): app(app_)
{
    _imgui = ImGuiIntegration::Context(Vector2{app->windowSize()}/app->dpiScaling(),
        app->windowSize(), app->framebufferSize());


    #if !defined(MAGNUM_TARGET_WEBGL) && !defined(CORRADE_TARGET_ANDROID)
    /* Have some sane speed, please */
    app->setMinimalLoopPeriod(16);
    #endif
}

void UI::drawEvent() {

    _imgui.newFrame();

    if(ImGui::GetIO().WantTextInput && !app->isTextInputActive())
        app->startTextInput();
    else if(!ImGui::GetIO().WantTextInput && app->isTextInputActive())
         app->stopTextInput();

    _imgui.updateApplicationCursor(*app);

    /* Set appropriate states. If you only draw ImGui, it is sufficient to
       just enable blending and scissor test in the constructor. */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    this->ui_loop();
    _imgui.drawFrame();

    /* Reset state. Only needed if you want to draw something else with
       different state after. */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

}

void UI::viewportEvent(Sdl2Application::ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    _imgui.relayout(Vector2{event.windowSize()}/event.dpiScaling(),
        event.windowSize(), event.framebufferSize());
}

void UI::keyPressEvent(Sdl2Application::KeyEvent& event) {
    if(_imgui.handleKeyPressEvent(event)) return;
}

void UI::keyReleaseEvent(Sdl2Application::KeyEvent& event) {
    if(_imgui.handleKeyReleaseEvent(event)) return;
}

bool UI::mousePressEvent(Sdl2Application::MouseEvent& event) {
    if(_imgui.handleMousePressEvent(event)) return true;
    return false;
}

void UI::mouseReleaseEvent(Sdl2Application::MouseEvent& event) {
    if(_imgui.handleMouseReleaseEvent(event)) return;
}

void UI::mouseMoveEvent(Sdl2Application::MouseMoveEvent& event) {
    if(_imgui.handleMouseMoveEvent(event)) return;
}

void UI::mouseScrollEvent(Sdl2Application::MouseScrollEvent& event) {
    if(_imgui.handleMouseScrollEvent(event)) {
        event.setAccepted();
        return;
    }
}

void UI::textInputEvent(Sdl2Application::TextInputEvent& event) {
    if(_imgui.handleTextInputEvent(event)) return;
}

 
