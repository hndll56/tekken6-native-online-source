#include "types.h"
#include <strings.h>
int Platform::GetConfigInt(const char* name) { const char* v = getenv(name); return v ? atoi(v) : 0; }
bool Platform::GetConfigBool(const char* name) { const char* v = getenv(name); return v && (atoi(v) != 0 || strcasecmp(v, "true") == 0); }
