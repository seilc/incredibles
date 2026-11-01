#ifndef DECOMP_H
#define DECOMP_H

/**
 * https://github.com/kiwi515/ogws/blob/master/include/decomp.h
 * Codewarrior tricks for matching decomp
 */

#define __CONCAT(x, y) x##y
#define CONCAT(x, y) __CONCAT(x, y)

// Reference a variable twice without code, so MWCC gives it a register
#define DECOMP_FORCEREGISTER(x) ((void)(x), (void)(x))

// Compile without matching hacks.
#if defined(NON_MATCHING) || !defined(__MWERKS__)
#define DECOMP_FORCEACTIVE(...)
#define DECOMP_FORCEFLOAT(f)
#define DECOMP_FORCEBLOCK(...)
// Compile with matching hacks.
// (This version of CW does not support pragmas inside macros.)
#else
// Force reference specific data
#define DECOMP_FORCEACTIVE(...)                                                          \
    static void fake_function(...);                                                      \
    static void CONCAT(FORCEACTIVE, __LINE__)(void) { fake_function(__VA_ARGS__); }

// Force float literal ordering in sdata2
#define DECOMP_FORCEFLOAT(f)                                                             \
    static void CONCAT(FORCELITERAL, __LINE__)(void) { *(float*)0 = (f); }

// For more complex forcing requirements
// Example usage: DECOMP_FORCEBLOCK((Class* dummy, int arg) {
//     dummy->Method(arg);
// })
#define DECOMP_FORCEBLOCK(...)                                                           \
    static void CONCAT(FORCEBLOCK, __LINE__) __VA_ARGS__
#endif

#endif
