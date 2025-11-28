#pragma once

#include <functional>

#include <QString>

namespace PluginFinder {

void doTheThing(const QString& subdir, const std::function<void(const QString&)>& callback);

} // namespace PluginFinder
