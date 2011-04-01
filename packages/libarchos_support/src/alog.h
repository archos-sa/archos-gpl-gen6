#ifndef ALOG_H
#define ALOG_H

/*
 * A very simple log system. It logs to stderr, but can also duplicate its
 * output to a file indicated by the ALOG_LOGFILE environment variable.
 */
#ifdef ALOG_ENABLE_DEBUG
#define ALOG_DEBUG(format, ...)   archos::log("DEBUG",   __FILE__, __LINE__, __FUNCTION__, format, ## __VA_ARGS__)
#define ALOG_DEBUG_FUNCTION_BLOCK archos::ALogDebugBlockHelper __alogDebugBlockHelper(__FILE__, __LINE__, __FUNCTION__)
#define ALOG_DEBUG_BLOCK(msg) archos::ALogDebugBlockHelper __alogDebugBlockHelper(__FILE__, __LINE__, (msg))
#else
#define ALOG_DEBUG(format, ...)
#define ALOG_DEBUG_FUNCTION_BLOCK
#define ALOG_DEBUG_BLOCK(msg)
#endif

#define ALOG_WARNING(format, ...) archos::log("WARNING", __FILE__, __LINE__, __FUNCTION__, format, ## __VA_ARGS__)

#define ALOG_ERROR(format, ...)   archos::log("ERROR",   __FILE__, __LINE__, __FUNCTION__, format, ## __VA_ARGS__)



namespace archos {

class ALogDebugBlockHelper {
public:
	ALogDebugBlockHelper(const char* file, int line, const char* message);
	~ALogDebugBlockHelper();

private:
	char* m_file;
	int   m_line;
	char* m_message;
};

void log(const char* level, const char* file, int line, const char* function, const char* format, ...);

} // namespace

#endif /* ALOG_H */
