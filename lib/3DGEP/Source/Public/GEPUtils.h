/*!
 * Project: First D3D12 Renderer - https://github.com/logins/FirstDX12Renderer
 *
 * File: GEPUtils.h
 *
 * Author: Riccardo Loggini
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

#define StopForFail(X) std::cout << X << std::endl; __debugbreak(); // This last will generate a breakpoint

#define DebugPrint(X) do {if(DEBUG_TEST) std::cout << X << std::endl;} while (0)

#define Check(X) if(DEBUG_TEST && !(X)) __debugbreak();

#define PrintD3dErrorBlob(X) std::cout << "Error Message: " << std::string((char*)(X->GetBufferPointer()),X->GetBufferSize()) << std::endl;
} 
#endif // GEPUtils_h__