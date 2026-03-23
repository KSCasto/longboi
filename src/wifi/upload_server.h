#pragma once

#include <Arduino.h>

// =============================================================================
// WiFi AP mode upload server
// =============================================================================

namespace UploadServer {

// Start AP mode and web server. Display shows upload instructions.
void start();

// Stop AP mode and web server. Frees resources.
void stop();

// Check if the server is currently running
bool isRunning();

}  // namespace UploadServer
