#pragma once

// Compatibility shim:
// KonataWriter now maps to LinxTraceWriter for the LinxCoreSight flow.
#include "pyc_linxtrace.hpp"

namespace pyc::cpp {
using KonataWriter = LinxTraceWriter;
}
