#include <AppCore/AppCore.h>
#include <iostream>

namespace ul = ultralight;

class App : public ul::AppListener, public ul::WindowListener, public ul::LoadListener, public ul::ViewListener {
    ul::RefPtr<ul::App> app;
    ul::RefPtr<ul::Window> window;
    ul::RefPtr<ul::Overlay> overlay;
public:
    App()
    : app{ ul::App::Create() }
    , window{ ul::Window::Create(app->main_monitor(), 800, 600, false, ul::kWindowFlags_Resizable | ul::kWindowFlags_Maximizable) }
    , overlay{ ul::Overlay::Create(window, 1, 1, 0, 0) }
    {
        app->set_listener(this);
        window->set_listener(this);
        overlay->view()->set_load_listener(this);
        overlay->view()->set_view_listener(this);

        window->MoveToCenter();
        overlay->Resize(window->width(), window->height());
        overlay->view()->LoadURL("file:///app.html");
        overlay->Focus();
    }

    inline void run() {
        app->Run();
    }

    void OnClose(ul::Window*) override {
        std::exit(0);
    }

    void OnResize(ul::Window*, uint32_t width, uint32_t height) override {
        overlay->Resize(width, height);
    }

    void OnDOMReady(ul::View *caller, uint64_t frame_id, bool is_main_frame, const ul::String &url) override {
        using ul::JSCallback, ul::JSCallbackWithRetval;
        ul::SetJSContext(caller->LockJSContext()->ctx());
        auto global = ul::JSGlobalObject();
    }

    void OnChangeCursor(ul::View *caller, ul::Cursor cursor) override {
        window->SetCursor(cursor);
    }

    void OnChangeTitle(ul::View *caller, const ul::String &title) override {
        window->SetTitle(title.utf8().data());
    }

    void OnAddConsoleMessage(ul::View*, ul::MessageSource, ul::MessageLevel, const ul::String &message,
                             uint32_t line_number, uint32_t column_number, const ul::String &) override {
        std::cout << "Console: " << message.utf8().data() << " at line: " << line_number << ", column: " << column_number << std::endl;
    }
};

auto main() -> int {
    App().run();
}
