#ifndef RUNTIME_H
#define RUNTIME_H

#if defined(QK_WINDOWS)

#include "windows/memory.h"
#include "windows/process.h"

#elif defined(QK_UNIX)

#include "unix/memory.h"
#include "unix/process.h"

#endif

#endif  // RUNTIME_H
