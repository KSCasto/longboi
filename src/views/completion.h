#pragma once

#include <Arduino.h>

// =============================================================================
// Completion screen — shown when the reader reaches the last page
// =============================================================================

enum class CompletionResult {
    MARK_AS_READ,
    KEEP_READING,
};

namespace Completion {

// Show the "You finished!" screen. Returns user's choice.
CompletionResult run(const char* bookFilename);

}  // namespace Completion
