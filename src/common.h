#ifdef VERBOSE
#define MESSAGE(...)     fprintf(stdout, __VA_ARGS__)
#define MESSAGE_ERR(...) fprintf(stderr, __VA_ARGS__)
#else
#define MESSAGE(...)
#define MESSAGE_ERR(...)
#endif