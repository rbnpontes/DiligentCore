#include "ReleaseCallback.h"

namespace Diligent
{

ReleaseCallbackType ReleaseCallback = nullptr;

void SetReleaseCallback(ReleaseCallbackType Callback)
{
	ReleaseCallback = Callback;
}

void ExecuteReleaseCallback(void* pObject, void* pRefCount)
{
	if((pObject || pRefCount) && ReleaseCallback != nullptr)
		ReleaseCallback(pObject, pRefCount);
}

} // namespace Diligent