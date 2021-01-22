/*
 GEPUtilsMath.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef GEPUtilsMath_h__
#define GEPUtilsMath_h__

namespace GEPUtils {
	namespace Math {

		// Note: InAlignmentUnit must be a power of two!
		// Aligns the input size to the input alignment unit.
		// How this works is: we first add InAlignUnit - 1 to the input size and then we "clean" all the bits interested by the alignment unit.
		// Since the InAlignmentUnit is a power of 2, by negating it, all the bits interested by the alignment will be 0 and all the rest will be 1.
		// In this way, the AND operation will place 0 (clean) all the bits interested by the alignment and leave the rest unchanged.
		// Since we added InAlignUnit - 1 at the beginning, the returned size will be always greater or equal to the input size.
		inline size_t Align(size_t InSize, size_t InAlignUnit) { return (InSize + InAlignUnit - 1) & ~(InAlignUnit - 1); }

		// Gotten from http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
		/*uint64_t NextPowerOfTwo(uint64_t InNumber) {
			InNumber--;
			InNumber |= InNumber >> 1;
			InNumber |= InNumber >> 2;
			InNumber |= InNumber >> 4;
			InNumber |= InNumber >> 8;
			InNumber |= InNumber >> 16;
			InNumber |= InNumber >> 32;
			InNumber |= InNumber >> 64;
			InNumber |= InNumber >> 128;
			InNumber |= InNumber >> 256;
			InNumber |= InNumber >> 512;
			InNumber++;

			return InNumber;
		}*/
	}
}
#endif // GEPUtilsMath_h__
