#ifdef VERBOSE
#define MESSAGE(...)          fprintf(stdout, __VA_ARGS__)
#define MESSAGE_ERR(...)      fprintf(stderr, __VA_ARGS__)
#else
#define MESSAGE(...)
#define MESSAGE_ERR(...)
#endif

#define IMAGE_FOLDER          "images"
#define JSON_FOLDER           "data"

#define ERR_NO_FILE           112
#define ERR_NO_INPUT_IMAGE    113
#define ERR_NO_INFO_JSON      114
#define ERR_WRONG_COMMANDLINE 115
#define ERR_NO_STREAM         116
#define ERR_THREAD_CREATION   117
#define ERROR_CLI             118
#define ERROR_VIDEO_STREAM    119
#define ERROR_DUMP_FRAME      120
