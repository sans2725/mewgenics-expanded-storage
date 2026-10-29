#ifndef ES_PREFLIGHT_H
#define ES_PREFLIGHT_H
#include "vault.h"
#include <wchar.h>
/* Returns 1 on an exact supported hash, 0 on mismatch, -1 on I/O/crypto failure.
 * This check does not establish runtime readiness or authorize any game write. */
ES_API int es_check_executable(const wchar_t *path, uint8_t actual_hash[32]);
ES_API const char *es_runtime_status(void);
#endif
