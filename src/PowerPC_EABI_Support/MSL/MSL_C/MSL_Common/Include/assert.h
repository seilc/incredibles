#ifdef __cplusplus
extern "C" {
#endif

extern void __msl_assertion_failed(char const *, char const *, char const *, int);

#ifdef __cplusplus
}
#endif

#ifdef assert
	#undef assert
#endif

#ifdef NDEBUG
    #define assert(ignore) ((void) 0)
#else
    #define assert(condition) ((condition) ? ((void) 0) : __msl_assertion_failed(#condition, __FILE__, __func__, __LINE__))
#endif
