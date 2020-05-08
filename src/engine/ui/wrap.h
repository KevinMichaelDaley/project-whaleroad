#include <Magnum/Math/Color.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>

#ifdef CORRADE_TARGET_ANDROID
#include <Magnum/Platform/AndroidApplication.h>
#elif defined(CORRADE_TARGET_EMSCRIPTEN)
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif
using namespace Magnum;

using namespace Math::Literals;
using namespace Magnum::Platform;
class UI{
    
    public:
        explicit UI(Platform::Application* app_);
        virtual void ui_loop(){}
        virtual Platform::Application* get_app(){ return app;}
        virtual void drawEvent();

        virtual void viewportEvent(Sdl2Application::ViewportEvent& event);

        virtual void keyPressEvent(Sdl2Application::KeyEvent& event);
        virtual void keyReleaseEvent(Sdl2Application::KeyEvent& event);

        virtual bool mousePressEvent(Sdl2Application::MouseEvent& event);
        virtual void mouseReleaseEvent(Sdl2Application::MouseEvent& event);
        virtual void mouseMoveEvent(Sdl2Application::MouseMoveEvent& event);
        virtual void mouseScrollEvent(Sdl2Application::MouseScrollEvent& event);
        virtual void textInputEvent(Sdl2Application::TextInputEvent& event);

    private:
        ImGuiIntegration::Context _imgui{NoCreate};
        Platform::Application* app;
};
