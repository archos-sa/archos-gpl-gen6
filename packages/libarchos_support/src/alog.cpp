#include "alog.h"

#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

namespace archos {

static FILE* g_logFile = 0;

static const char* ALOG_LOGFILE_ENV_VAR = "ALOG_LOGFILE";


ALogDebugBlockHelper::ALogDebugBlockHelper(const char* file, int line, const char* message)
{
	m_file = strdup(file);
	m_line = line;
	m_message = strdup(message);
	log("DEBUG", m_file, m_line, m_message, "Entering");
}


ALogDebugBlockHelper::~ALogDebugBlockHelper()
{
	log("DEBUG", m_file, m_line, m_message, "Leaving");
	free(m_file);
	free(m_message);
}


static void init()
{
	static bool firstCall = true;
	if (!firstCall) {
		return;
	}
	firstCall = false;

	const char* filename = getenv(ALOG_LOGFILE_ENV_VAR);
	if (!filename) {
		return;
	}

	g_logFile = fopen(filename, "w");
	if (!g_logFile) {
		// This should not cause reentrancy troubles, since firstCall has been
		// set to false
		ALOG_WARNING("Could not create logfile '%s'", filename);
	}
}


inline void doLog(const char* buffer)
{
	if (g_logFile) {
		fprintf(g_logFile, buffer);
	}
	fprintf(stderr, buffer);
}


void log(const char* level, const char* file, int line, const char* function, const char* format, ...)
{
	init();
	// Do not make this buffer static, otherwise the function won't be thread
	// safe
	char buffer[256];
	int length;
	snprintf(buffer, sizeof buffer - 1, "[%s] (%s:%d) %s: ", level, file, line, function);
	doLog(buffer);

	va_list ap;
	va_start(ap, format);
	length = vsnprintf(buffer, sizeof buffer - 1, format ,ap);
	va_end(ap);
	doLog(buffer);

	doLog("\n");

	fflush(stderr);
	if (g_logFile) {
		fflush(g_logFile);
		sync();
	}
}

} // namespace
