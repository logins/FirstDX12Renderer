/*
 GEPUtils.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef GEPUtils_h__
#define GEPUtils_h__

#ifdef _DEBUG
#define DEBUG_TEST 1
#include<iostream>
#else
#define DEBUG_TEST 0
#endif

namespace GEPUtils {

	namespace Constants {
		static constexpr size_t g_MaxConcurrentFramesNum = 2;

	}

	// In a bigger application this would go in an Input class
	enum class KEYBOARD_KEY : uint32_t
	{
		KEY_V,
		KEY_ESC
	};

#define StopForFail(X) std::cout << X << std::endl; __debugbreak(); // This last will generate a breakpoint

#define DebugPrint(X) do {if(DEBUG_TEST) std::cout << X << std::endl;} while (0)

#define Check(X) if(DEBUG_TEST && !(X)) __debugbreak();

#define PrintD3dErrorBlob(X) std::cout << "Error Message: " << std::string((char*)(X->GetBufferPointer()),X->GetBufferSize()) << std::endl;
} 
#endif // GEPUtils_h__