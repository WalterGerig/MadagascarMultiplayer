#pragma once

namespace madloader {

// Starts the patch-loading worker thread. Safe to call from DllMain: it only
// records paths and spawns a thread. All LoadLibrary work happens on that
// thread, because calling LoadLibrary under the loader lock is a documented
// deadlock hazard.
void Start(const char* how);

void Log(const char* fmt, ...);

} // namespace madloader
