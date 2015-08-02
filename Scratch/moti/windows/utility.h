#pragma once

#include <intsafe.h>

namespace moti { namespace windows { namespace utility {
	void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		}
	}
}}}
