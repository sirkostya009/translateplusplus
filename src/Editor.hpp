#ifndef EDITOR_HPP
#define EDITOR_HPP

#include <AppCore/AppCore.h>
#include <string>
#include <fstream>
#include <sstream>

namespace ul = ultralight;

class Editor : public ul::WindowListener, public ul::LoadListener, public ul::ViewListener {
    ul::RefPtr<ul::Window> window;
    ul::RefPtr<ul::Overlay> overlay;

    std::function<void()> onClose;

    std::string filename;
public:
    Editor(ul::Monitor* monitor, std::function<void()> onClose, std::string title)
    : window{ ul::Window::Create(monitor, 800, 600, false, ul::kWindowFlags_Resizable | ul::kWindowFlags_Maximizable) }
    , overlay{ ul::Overlay::Create(window, 1, 1, 0, 0) }
    , onClose{ std::move(onClose) }
    , filename{std::move(title) }
    {
        window->set_listener(this);
        overlay->view()->set_load_listener(this);
        overlay->view()->set_view_listener(this);

        window->MoveToCenter();
        overlay->Resize(window->width(), window->height());
#include "resources/editor.inl"
        overlay->view()->LoadHTML(rawData);
        overlay->Focus();
        window->SetTitle(filename.c_str());
    }

    void OnClose(ultralight::Window *w) override {
        w->Close();
        std::ofstream(filename) << ((ul::String) overlay->view()->EvaluateScript("dict.value")).utf8().data();
        onClose();
    }

    void OnResize(ultralight::Window *, uint32_t width_px, uint32_t height_px) override {
        overlay->Resize(width_px, height_px);
    }

    bool OnKeyEvent(const ultralight::KeyEvent &evt) override {
        switch (evt.virtual_key_code) {
            case 27: // Escape
                OnClose(window.get());
                break;
        }
        return true;
    }

    void OnDOMReady(ultralight::View *caller, uint64_t frame_id, bool is_main_frame, const ultralight::String &url) override {
        auto fs = std::ifstream(filename);
        auto ss = std::stringstream();
        ss << "dict.value = `";
        for (auto buf = std::string(); std::getline(fs, buf);) {
            ss << buf << '\n';
        }
        ss << '`';

        caller->EvaluateScript(ss.str().c_str());
    }
};

#endif
