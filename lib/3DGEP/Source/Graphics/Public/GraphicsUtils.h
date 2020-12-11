#ifndef GraphicsUtils_h__
#define GraphicsUtils_h__
#include "GraphicsTypes.h"
#include <memory> // for std::unique_ptr

// TODO move this in general Utils (not graphics specific)
// this has taken inspiration from DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) defined in winnt.h
#define DEFINE_OPERATORS_FOR_ENUM(ENUMTYPE) \
inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return static_cast<ENUMTYPE>((int)a | b);} \
inline ENUMTYPE &operator |= (ENUMTYPE &a, ENUMTYPE b){return a = static_cast<ENUMTYPE>((int)a | b);} \
inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b){return static_cast<ENUMTYPE>((int)a & b);} \
inline ENUMTYPE operator &= (ENUMTYPE &a, ENUMTYPE b){return a = static_cast<ENUMTYPE>((int)a & b);}


namespace GEPUtils { namespace Graphics {

	class PipelineState;
	class Device;

	// Note: If we had another SDK to choose from, the same functions would be defined again returning objects from the other SDK

	void EnableDebugLayer();

	DynamicBuffer& AllocateDynamicBuffer();

	VertexBufferView& AllocateVertexBufferView();
	IndexBufferView& AllocateIndexBufferView();
	ConstantBufferView& AllocateConstantBufferView(GEPUtils::Graphics::Buffer& InResource, GEPUtils::Graphics::RESOURCE_VIEW_TYPE InType);

	Shader& AllocateShader(wchar_t const* InShaderPath);

	PipelineState& AllocatePipelineState();

	std::unique_ptr<Rect> AllocateRect(int32_t InLeft, int32_t InTop, int32_t InRight, int32_t InBottom);

	std::unique_ptr<ViewPort> AllocateViewport(float InTopLeftX, float InTopLeftY, float InWidth, float InHeight);


} }
#endif // GraphicsUtils_h__
