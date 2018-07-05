#pragma once

// This header-file includes <libwebsockets.h> but disables it warnings.
//
// When using GCC6 and later we cannot include it using -isystem since that
// messes with the system include order causing compilation errors.
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <libwebsockets.h>
#pragma GCC diagnostic pop
#endif
