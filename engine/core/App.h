#include "core/Window.h"
#include "renderer/Renderer.h"

class App
{
public:
    App(int width = 800, int height = 600, const std::string& title = "XVEngine");
    ~App();

    void Run();

private:
    Window m_Window;
    Renderer m_Renderer;
};