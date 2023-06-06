#if defined(DOMCOLOR_API_BEING_BUILT)
#    if defined(_MSC_VER)
#        define DOMCOLOR_API __declspec(dllexport)
#    else
#        define DOMCOLOR_API __attribute__((visibility("default")))
#    endif
#elif defined(DOMCOLOR_API_BEING_IMPORTED)
#    if defined(_MSC_VER)
#        define DOMCOLOR_API __declspec(dllimport)
#    else
#        define DOMCOLOR_API __attribute__((visibility("default")))
#    endif
#else
#define DOMCOLOR_API
#endif
