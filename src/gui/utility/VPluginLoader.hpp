#pragma once

#include <functional>

#include <QString>

namespace VPluginLoader {

void iterate(const QString& subdir, const std::function<void(const QString&)>& callback);

} // namespace PluginFinder
