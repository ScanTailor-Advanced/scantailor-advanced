// IosDirectoryPicker.h
#pragma once
#include <QString>
#include <functional>

// Shows the native iOS folder/file picker asynchronously.
// The callback is called on the main thread with the selected directory path,
// or an empty string if cancelled.
void iosPickDirectory(const QString& title, std::function<void(const QString&)> callback);
