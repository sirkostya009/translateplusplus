#ifndef C_EDIT_ASSETSPROVIDER_H
#define C_EDIT_ASSETSPROVIDER_H

#include <Ultralight/Ultralight.h>
#include <string_view>
#include "assets.h"

namespace ul = ultralight;

class AssetsProvider : public ul::FileSystem {
public:
    bool FileExists(const ul::String &file_path) override {
        return true;
    }

    ul::String GetFileMimeType(const ul::String &file_path) override {
        const auto view = std::string_view(file_path.utf8().data());

        if (view.ends_with(".pem")) {
            return "application/x-x509-ca-cert";
        } else if (view.ends_with(".dat")) {
            return "application/octet-stream";
        }

        return {};
    }

    ul::String GetFileCharset(const ul::String &file_path) override {
        return "utf-8";
    }

    ul::RefPtr<ul::Buffer> OpenFile(const ul::String &file_path) override {
        const auto view = std::string_view(file_path.utf8().data());

        if (view.ends_with(".pem")) {
            return ul::Buffer::Create(cacert, sizeof(cacert), {}, {});
        } else if (view.ends_with(".dat")) {
            return ul::Buffer::Create(icudt67l, sizeof(icudt67l), {}, {});
        }

        return {};
    }
};


#endif //C_EDIT_ASSETSPROVIDER_H
