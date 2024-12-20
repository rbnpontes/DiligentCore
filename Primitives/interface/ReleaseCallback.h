#pragma once

#include "BasicTypes.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

/// Type of the release callback function

/// \param [in] pObject - Object that is going to be release
typedef void(DILIGENT_CALL_TYPE* ReleaseCallbackType)(void* pObject, void* pRefCount);

extern ReleaseCallbackType ReleaseCallback;

/// Sets the release callback function

/// \note This functions needs to be called once to register
///		  a callback. When callback is set, every time that
///		  object goes to be release the callback will be called.
void SetReleaseCallback(ReleaseCallbackType Callback);

/// Execute release callback function

/// \note This functions executes release callback
///		  from a given pObject.
void ExecuteReleaseCallback(void* pObject, void* pRefCount);

DILIGENT_END_NAMESPACE // namespace Diligent