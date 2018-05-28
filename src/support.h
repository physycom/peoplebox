#ifdef VERBOSE
#define MESSAGE(...)     fprintf(stdout, __VA_ARGS__)
#define MESSAGE_ERR(...) fprintf(stderr, __VA_ARGS__)
#else
#define MESSAGE(...)
#define MESSAGE_ERR(...)
#endif

#define ERR_NO_FILE             11
#define ERR_NO_STREAM           12
#define ERR_NO_INFO_JSON        13
#define ERR_THREAD_CREATION     22
#define ERR_NO_NAMELIST_FILE    33
