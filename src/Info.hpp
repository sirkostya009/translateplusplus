#ifndef INFO_HPP
#define INFO_HPP

#include <AppCore/AppCore.h>

namespace ul = ultralight;

class Info : public ul::WindowListener, public ul::LoadListener, public ul::ViewListener {
    ul::RefPtr<ul::Window> window;
    ul::RefPtr<ul::Overlay> overlay;

    std::function<void()> onClose;
public:
    Info(ul::Monitor* monitor, std::function<void()> onClose)
    : window{ ul::Window::Create(monitor, 850, 300, false, ul::kWindowFlags_Resizable | ul::kWindowFlags_Maximizable) }
    , overlay{ ul::Overlay::Create(window, 1, 1, 0, 0) }
    , onClose{ std::move(onClose) }
    {
        window->set_listener(this);
        overlay->view()->set_load_listener(this);
        overlay->view()->set_view_listener(this);

        window->MoveToCenter();
        overlay->Resize(window->width(), window->height());
#include "resources/info.inl"
        overlay->view()->LoadHTML(rawData);
        overlay->Focus();
    }

    void OnChangeTitle(ultralight::View *caller, const ultralight::String &title) override {
        window->SetTitle(title.utf8().data());
    }

    void OnResize(ul::Window *, uint32_t width_px, uint32_t height_px) override {
        overlay->Resize(width_px, height_px);
    }

    void OnClose(ul::Window *w) override {
        w->Close();
        onClose();
    }

    bool OnKeyEvent(const ul::KeyEvent &evt) override {
        switch (evt.virtual_key_code) {
            case 27: // Escape
                OnClose(window.get());
                break;
        }
        return true;
    }
};

#endif
