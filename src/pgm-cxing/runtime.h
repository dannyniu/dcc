/* DannyNiu/NJF, 2025-10-02. Public Domain. */

#ifndef cxing2c_runtime_h
#define cxing2c_runtime_h 1

// A few stuff specific to the reference implementation.

#include "value-nativobj.h"

/// @fn
/// @param msg The diagnostic message.
/// @details
/// Diagnosis are for severe things such as violations of runtime assumptions.
void CxingDiagnose(const char *msg);

/// @fn
/// @param msg The warning message.
/// @details
/// Warnings indicates current or potential future degradation of service.
/// These types of degradation shouldn't be functionally affecting.
void CxingWarning(const char *msg);

/// @fn
/// @param msg The error message.
/// @details
/// Fatal errors prevents the program from correctly functioning.
void CxingFatal(const char *msg);

extern struct value_nativeobj CxingPropName_copy;
extern struct value_nativeobj CxingPropName_final;

bool CxingRuntimeInit();

#endif /* cxing2c_runtime_h */
