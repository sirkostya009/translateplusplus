#include <AppCore/AppCore.h>
#include <iostream>
#include <chrono>
#include <map>
#include <string>
#include <regex>
#include <utility>
#include "json.hpp"
#include <windows.h>
#include <shobjidl.h>
#include <fstream>

namespace ul = ultralight;

using json = nlohmann::json;

class Editor : public ul::WindowListener, public ul::LoadListener, public ul::ViewListener {
    ul::RefPtr<ul::Window> window;
    ul::RefPtr<ul::Overlay> overlay;

    std::function<void()> onClose;

    std::string filename;
public:
    Editor(ul::Monitor* monitor, std::function<void()> onClose, std::string title = "Editor")
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
        overlay->view()->LoadURL("file:///editor.html");
        overlay->Focus();
        window->SetTitle(filename.c_str());
    }

    void OnClose(ultralight::Window *w) override {
        w->Close();
        std::ofstream(filename) << ((ul::String) overlay->view()->EvaluateScript("textarea.value")).utf8().data();
        onClose();
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
        ss << "textarea.value = `";
        for (auto buf = std::string(); std::getline(fs, buf);) {
            ss << buf << '\n';
        }
        ss << '`';

        std::cout << ss.str() << std::endl;

        caller->EvaluateScript(ss.str().c_str());
    }
};

class App : public ul::AppListener, public ul::WindowListener, public ul::LoadListener, public ul::ViewListener {
    ul::RefPtr<ul::App> app;
    ul::RefPtr<ul::Window> window;
    ul::RefPtr<ul::Overlay> overlay;

    std::chrono::milliseconds lastPush = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    struct ci_less {
        // case-independent (ci) compare_less binary function
        struct nocase_compare {
            bool operator() (const unsigned char& c1, const unsigned char& c2) const {
                return tolower (c1) < tolower (c2);
            }
        };
        bool operator() (const std::string & s1, const std::string & s2) const {
            return std::lexicographical_compare
                    (s1.begin (), s1.end (),   // source range
                     s2.begin (), s2.end (),   // dest range
                     nocase_compare ());  // comparison
        }
    };

    std::map<std::string, std::string, ci_less> dictionary;
    std::regex word_regex{R"(\w+|\s{2,}|\n|\r|\t)"};

    json config{
        {"file", "basic.json"},
        {"dictionaries", {
        }}
    };

    ul::JSFunction addDictionaryOption;

    Editor* editor{};
    bool rizz = false;
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

    void OnUpdate() override {
        if (rizz) {
            dictionary = json::parse(std::ifstream(std::string(config["file"])));
            rizz = false;
            delete editor;
        }
    }

    bool OnKeyEvent(const ul::KeyEvent &evt) override {
        switch (evt.virtual_key_code) {
        case 112: // F1
            break;
        case 13: // Enter
            ul::JSEval("process(input.value)");
            break;
        }
        return true;
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

        global["openNewDictionary"] = BindJSCallback(&App::openNewDictionary);
        global["loadDictionary"]    = BindJSCallback(&App::loadDictionary);
        global["openEditor"]        = BindJSCallback(&App::openEditor);
        global["process"]           = BindJSCallback(&App::process);
        global["copy"]              = BindJSCallback(&App::copy);

        addDictionaryOption = global["addDictionaryOption"];
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

    void process(const ul::JSObject&, const ul::JSArgs& args) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        if ((lastPush - now).count() > -500) {
            return;
        }
        lastPush = now;

        auto string = std::string(((ul::String) args[0]).utf8().data());
        auto result = std::string();

        for (auto match = std::smatch(); std::regex_search(string, match, word_regex); string = match.suffix()) {
            auto word = match.str();

            result += (dictionary.find(word) != dictionary.end() ? dictionary[word] : word);

            if (result[result.size() - 1] != '\n'
             || result[result.size() - 1] != '\r'
             || result[result.size() - 1] != '\t'
             || result[result.size() - 1] != ' ') {
                result += ' ';
            }
        }

        ul::JSEval(("result.value = `" + result + '`').c_str());
    }

    void copy(const ul::JSObject&, const ul::JSArgs& args) {
        auto text = ((ul::String) args[0]).utf8().data();
    }

    void loadDictionary(const ul::JSObject&, const ul::JSArgs&) {
    }

    void openNewDictionary(const ul::JSObject& self, const ul::JSArgs& args) {
        IFileOpenDialog *dialog;
        auto hr = CoCreateInstance(
                CLSID_FileOpenDialog,
                nullptr,
                CLSCTX_ALL,
                IID_IFileOpenDialog,
                reinterpret_cast<LPVOID*>(&dialog)
        );
        if (FAILED(hr)) {
            return;
        }

        auto handle = (HWND) window->native_handle();

        auto jsonFilter = COMDLG_FILTERSPEC{L"JSON Files", L"*.json"};
        dialog->SetFileTypes(1, &jsonFilter);

        hr = dialog->Show(handle);
        if (FAILED(hr)) {
            return;
        }

        IShellItem *item;
        hr = dialog->GetResult(&item);
        if (FAILED(hr)) {
            return;
        }

        LPWSTR filePath;
        hr = item->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
        if (FAILED(hr)) {
            return;
        }

        CoTaskMemFree(filePath);
        item->Release();
        dialog->Release();

        try {
            dictionary = json::parse(std::ifstream(filePath));
            config["file"] = filePath;
            char buffer[500];
            wcstombs(buffer, filePath, 500);
            addDictionaryOption({buffer});
        } catch (const std::exception& e) {
            std::cerr << "Failed to load dictionary\n" << e.what() << std::endl;
        }
    }

    void openEditor(const ul::JSObject&, const ul::JSArgs& args) {
        editor = new Editor(app->main_monitor(),
                            [this] { rizz = true; },
                            config["file"]);
    }
};

auto main() -> int {
    CoInitialize(nullptr);

    App().run();
}