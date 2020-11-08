/*!
 * Project: First D3D12 Renderer - https://github.com/logins/FirstDX12Renderer
 *
 * File: D3D12BufferAllocator.h
 *
 * Author: Riccardo Loggini
 */
#ifndef D3D12BufferAllocator_h__
#define D3D12BufferAllocator_h__

#include <deque>
#include "d3dx12.h"

namespace GEPUtils{ namespace Graphics {

/*!
 * \class D3D12BufferAllocator
 *
 * \brief Provides the ability to upload dynamic constant, vertex and index buffer data to the GPU. It allocates buffers on GPU memory pages in a linear manner.
 * Inspired to the UploadBuffer class by 3DGEP Lesson 3 https://www.3dgep.com/learning-directx-12-3/#Upload_Buffer
 *
 * - Allocates a resource in GPU shared memory (upload heap) per each memory page.
 * 
 * - The advantage of using this class is that the buffers will be allocated once, and then re-used with different data upon the next frame, instead of re-creating the entire buffers all over again.
 * - In out case vertex and index data do not change from frame to frame, but this type of container can come particularly useful when uploading uniform data to a constant buffer used in shader (e.g. MVP matrix)
 * - or when handling particles data simulated in CPU: instead of creating upload buffers on every frame, we can re-use the same buffers of the previous frame and upload different data. 
 * 
 * \author Riccardo Loggini
 * \date October 2020
 */
	class D3D12BufferAllocator {

	public:

		explicit D3D12BufferAllocator(size_t InPageSizeBytes = m_DefaultPageSize);

		// Allocation object to upload data to the GPU
		struct Allocation {
			void* CPU;
			D3D12_GPU_VIRTUAL_ADDRESS GPU;
		};

		// Note: the maximum allocation size will be equal to the page size
		size_t GetPageSize() const { return m_PageSize; }

		bool AllocateIfPossible(size_t InSizeBytes, size_t InAlignmentBytes, D3D12BufferAllocator::Allocation& OutAllocation);

		// Releases all the allocated pages. It should be done when the command list finishes executing on the command queue
		void Reset();

	private:

		struct Page {
			Page(size_t InSizeBytes);
			~Page();
			bool HasSpace(size_t InSizeBytes, size_t InAlignmentBytes);
			bool AllocateIfPossible(size_t InSizeBytes, size_t InAlignmentBytes, D3D12BufferAllocator::Allocation& OutAllocation);
			// Reset page for reuse
			void Reset();

		private:
			Microsoft::WRL::ComPtr<ID3D12Resource> m_D3d12Resource;
			
			void* m_CpuBasePtr = nullptr;
			D3D12_GPU_VIRTUAL_ADDRESS m_GpuBasePtr = D3D12_GPU_VIRTUAL_ADDRESS(0);

			// Offset from the base pointer from which we will find free memory
			size_t m_TakenSpaceOffset = 0;
			size_t m_PageSize;
		};

		size_t m_PageSize; // Size used to create a page

		// Retrieves a free page or creates a new one if all the current ones are in use
		Page& RequestPage();

		// A pool of memory pages.
		using PagePool = std::deque<std::unique_ptr<Page>>;

		PagePool m_PagesPool;
		size_t m_FreePagesNum = 0;

		Page& m_CurrentPage;

		static const size_t m_DefaultPageSize = 2000;
	};


} }

#endif // D3D12BufferAllocator_h__

