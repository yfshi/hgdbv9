#ifndef HGA_LOGGER_FE_H
#define HGA_LOGGER_FE_H

extern bool CreateLogger(char *logdir, int segsize, char *mailuser);
extern void LoggerLog(int elevel, const char *fmt,...);
extern void LoggerClose(void);

#endif /* HGA_LOGGER_FE_H */