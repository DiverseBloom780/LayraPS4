#include <vector>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/error.h"
#include "common/io_file.h"
#include "common/logging/log.h"
#include "common/path_util.h"

#ifdef _WIN32
#include "common/ntapi.h"

#include <io.h>
#include <share.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef _MSC_VER
#define fileno _fileno
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

namespace Common::FS {

namespace fs = std::filesystem;

namespace {

#ifdef _WIN32

[[nodiscard]] constexpr const wchar_t* AccessModeToWStr(FileAccessMode mode, FileType type) {
    switch (type) {
    case FileType::BinaryFile:
        switch (mode) {
        case FileAccessMode::Read:
            return L"rb";
        case FileAccessMode::Append:
            return L"ab";
        case FileAccessMode::Write:
        case FileAccessMode::ReadWrite:
            return L"r+b";
        case FileAccessMode::ReadAppend:
            return L"a+b";

        case FileAccessMode::Create:
