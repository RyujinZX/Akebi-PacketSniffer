#ifndef PCH_H
#define PCH_H

#include <magic_enum.hpp>

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <sstream>
#include <mutex>
#include <optional>
#include <atomic>
#include <regex>
#include <chrono>
#include <thread>

#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <tchar.h>

#include <imgui.h>
#include <imconfig.h>
#include <misc/cpp/imgui_stdlib.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <cheat-base/Logger.h>

#include <cheat-base/events/event.hpp>
#include <cheat-base/events/handlers/methodeventhandler.hpp>
#include <cheat-base/events/handlers/functoreventhandler.hpp>
#include <cheat-base/events/joins/handlereventjoin.hpp>
#include <cheat-base/events/joins/eventjoinwrapper.hpp>

#include <cheat-base/config/Config.h>
#include <cheat-base/util.h>
#include <cheat-base/PipeTransfer.h>
#include <cheat-base/thread-safe.h>

#include <cheat-base/render/gui-util.h>

#include <TextEditor.h>
#include <lua.hpp>
#endif //PCH_H