
#ifndef NONNULL
#if defined(__GNUC__) || defined(__clang__)
#define NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#else
#define NONNULL(...)
#endif
#endif

#ifndef ALL_NONNULL
#if defined(__GNUC__) || defined(__clang__)
#define ALL_NONNULL __attribute__((nonnull))
#else
#define ALL_NONNULL
#endif
#endif