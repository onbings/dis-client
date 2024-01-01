
#ifndef BOFSTD_EXPORT_H
#define BOFSTD_EXPORT_H

#ifdef BOFSTD_STATIC_DEFINE
#  define BOFSTD_EXPORT
#  define BOFSTD_NO_EXPORT
#else
#  ifndef BOFSTD_EXPORT
#    ifdef bofstd_EXPORTS
        /* We are building this library */
#      define BOFSTD_EXPORT 
#    else
        /* We are using this library */
#      define BOFSTD_EXPORT 
#    endif
#  endif

#  ifndef BOFSTD_NO_EXPORT
#    define BOFSTD_NO_EXPORT 
#  endif
#endif

#ifndef BOFSTD_DEPRECATED
#  define BOFSTD_DEPRECATED __declspec(deprecated)
#endif

#ifndef BOFSTD_DEPRECATED_EXPORT
#  define BOFSTD_DEPRECATED_EXPORT BOFSTD_EXPORT BOFSTD_DEPRECATED
#endif

#ifndef BOFSTD_DEPRECATED_NO_EXPORT
#  define BOFSTD_DEPRECATED_NO_EXPORT BOFSTD_NO_EXPORT BOFSTD_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef BOFSTD_NO_DEPRECATED
#    define BOFSTD_NO_DEPRECATED
#  endif
#endif

#endif /* BOFSTD_EXPORT_H */
