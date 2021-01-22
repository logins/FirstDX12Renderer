/*
 Helpers.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef Helpers_h__
#define Helpers_h__

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

#endif // Helpers_h__
