#ifndef APP_HPP
#define APP_HPP

#include <AppCore/AppCore.h>
#include <map>
#include <regex>
#include "json.hpp"
#include <windows.h>
#include <shobjidl.h>
#include <iostream>

#include "Editor.hpp"
#include "Info.hpp"
#include "Translator.hpp"

namespace ul = ultralight;

using json = nlohmann::json;

class App : public ul::AppListener, public ul::WindowListener, public ul::LoadListener, public ul::ViewListener {
    ul::RefPtr<ul::App> app;
    ul::RefPtr<ul::Window> window;
    ul::RefPtr<ul::Overlay> overlay;

    json config{
        {"file", "basic.json"},
        {"dictionaries", {}}
    };

    ul::JSFunction addDictionaryOption;

    std::unique_ptr<Editor> editor;
    bool close_editor = false;

    std::unique_ptr<Info> info_ptr;
    bool close_info = false;

    Translator translator;
public:
    App()
    : app{ ul::App::Create() }
    , window{ ul::Window::Create(app->main_monitor(), 800, 600, false, ul::kWindowFlags_Resizable | ul::kWindowFlags_Maximizable) }
    , overlay{ ul::Overlay::Create(window, 1, 1, 0, 0) }
    {
        CoInitialize(nullptr);

        app->set_listener(this);
        window->set_listener(this);
        overlay->view()->set_load_listener(this);
        overlay->view()->set_view_listener(this);

        window->MoveToCenter();
        overlay->Resize(window->width(), window->height());
#include "resources/app.inl"
        overlay->view()->LoadHTML(rawData);
        overlay->Focus();

        if (auto fs = std::ifstream("config.json"); fs) {
            fs >> config;

            trySetDictionary(config["file"]);
        }
    }

    inline void run() {
        app->Run();
    }

    void OnUpdate() override {
        if (close_editor) {
            trySetDictionary(config["file"]);
            editor = nullptr;
            close_editor = false;
        }

        if (close_info) {
            info_ptr = nullptr;
            close_info = false;
        }
    }

    bool OnKeyEvent(const ul::KeyEvent &evt) override {
        switch (evt.virtual_key_code) {
            case 112: // F1
                openInfo({}, {});
                break;
        }
        return true;
    }

    void OnClose(ul::Window*) override {
        if (auto fs = std::ofstream("config.json"); fs) {
            fs << config;
        }

        std::exit(0);
    }

    void OnResize(ul::Window*, uint32_t width, uint32_t height) override {
        overlay->Resize(width, height);
    }

    void OnDOMReady(ul::View *caller, uint64_t frame_id, bool is_main_frame, const ul::String &url) override {
        using ul::JSCallback, ul::JSCallbackWithRetval;
        ul::SetJSContext(caller->LockJSContext()->ctx());
        auto global = ul::JSGlobalObject();

        global["openNewDictionary"] = BindJSCallback(&App::openNewDictionary);
        global["loadDictionary"]    = BindJSCallback(&App::loadDictionary);
        global["translate_file"]     = BindJSCallback(&App::translateFile);
        global["openEditor"]        = BindJSCallback(&App::openEditor);
        global["openInfo"]          = BindJSCallback(&App::openInfo);
        global["process"]           = BindJSCallbackWithRetval(&App::process);
        global["copy"]              = BindJSCallback(&App::copy);
        global["nuke"]              = JSCallback([this](const ul::JSObject&, const ul::JSArgs&) { OnClose(window.get()); });

        addDictionaryOption = global["addDictionaryOption"];
        for (auto& file : config["dictionaries"]) {
            addDictionaryOption({((std::string)file).c_str()});
        }
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

    LPWSTR openFile(size_t filterSize, COMDLG_FILTERSPEC* filters) {
        IFileOpenDialog *dialog;
        auto hr = CoCreateInstance(
                CLSID_FileOpenDialog,
                nullptr,
                CLSCTX_ALL,
                IID_IFileOpenDialog,
                reinterpret_cast<LPVOID*>(&dialog)
        );
        if (FAILED(hr)) {
            return nullptr;
        }

        auto handle = (HWND) window->native_handle();

        dialog->SetFileTypes(filterSize, filters);

        hr = dialog->Show(handle);
        if (FAILED(hr)) {
            return nullptr;
        }

        IShellItem *item;
        hr = dialog->GetResult(&item);
        if (FAILED(hr)) {
            return nullptr;
        }

        LPWSTR filePath;
        hr = item->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
        if (FAILED(hr)) {
            return nullptr;
        }

        item->Release();
        dialog->Release();
        CoTaskMemFree(filePath);

        return filePath;
    }

    ul::JSValue process(const ul::JSObject&, const ul::JSArgs& args) {
        auto result = translator.translate_sentence(((ul::String) args[0]).utf8().data());
        auto ulString = ul::String(result.c_str()).utf32();

        ul::JSEval(("result.value = `" + result + '`').c_str());
        auto count = std::to_string(ulString.empty() ? ulString.length() : ulString.length() - 1);
        ul::JSEval(("resultCount.innerText = resultCounter.innerText = " + count).c_str());
        return {((ul::String) args[0]).utf8().length()};
    }

    void translateFile(const ul::JSObject&, const ul::JSArgs& args) {
        auto filter = COMDLG_FILTERSPEC{L"Text Files", L"*.txt"};
        auto filePath = openFile(1, &filter);
        if (!filePath) {
            return;
        }

        auto in = std::wstring(filePath);

        filePath = openFile(1, &filter);
        if (!filePath) {
            return;
        }

        auto out = std::wstring(filePath);

        translator.translate_file({in.begin(), in.end()}, {out.begin(), out.end()});
    }

    void copy(const ul::JSObject&, const ul::JSArgs& args) {
        auto text = ((ul::String) args[0]).utf8();
        auto hwnd = (HWND) window->native_handle();

        OpenClipboard(hwnd);
        EmptyClipboard();
        auto hg = GlobalAlloc(GMEM_MOVEABLE, text.length() + 1);
        if (!hg) {
            CloseClipboard();
            return;
        }
        memcpy(GlobalLock(hg), text.data(), text.length() + 1);
        GlobalUnlock(hg);
        SetClipboardData(CF_TEXT,hg);
        CloseClipboard();
        GlobalFree(hg);
    }

    void loadDictionary(const ul::JSObject&, const ul::JSArgs& args) {
        auto file = ((ul::String) args[0]).utf8().data();

        if (trySetDictionary(file)) {
        }
    }

    void openNewDictionary(const ul::JSObject&, const ul::JSArgs&) {
        auto filter = COMDLG_FILTERSPEC{L"JSON Files", L"*.json"};
        auto filePath = openFile(1, &filter);
        if (!filePath) {
            return;
        }

        auto wstr = std::wstring(filePath);
        auto str = std::string(wstr.begin(), wstr.end());

        if (trySetDictionary(str)) {
            if (!config["dictionaries"].contains(str)) {
                config["dictionaries"] += filePath;
            }
            addDictionaryOption({str.c_str()});
        }
    }

    void openEditor(const ul::JSObject&, const ul::JSArgs&) {
        editor = std::make_unique<Editor>(app->main_monitor(), [this] { close_editor = true; }, config["file"]);
    }

    void openInfo(const ul::JSObject&, const ul::JSArgs&) {
        info_ptr = std::make_unique<Info>(app->main_monitor(), [this] { close_info = true; });
    }

    bool trySetDictionary(const std::string& path) {
        try {
            translator.set_dictionary(json::parse(std::ifstream(path)));
            config["file"] = path;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to load dictionary\n" << e.what() << std::endl;
        }
        return false;
    }
};

#endif
