#ifndef GraphicsTypes_h__
#define GraphicsTypes_h__
#include <type_traits>
#include <string>
namespace GEPUtils { namespace Graphics {

	enum class PRIMITIVE_TOPOLOGY_TYPE : int32_t 
	{
		PTT_UNDEFINED = 0,
		PTT_POINT = 1,
		PTT_LINE = 2,
		PTT_TRIANGLE = 3,
		PTT_PATCH = 4
	};

	enum class PRIMITIVE_TOPOLOGY : int32_t
	{
		PT_UNDEFINED = 0,
		PT_POINTLIST = 1,
		PT_LINELIST = 2,
		PT_LINESTRIP = 3,
		PT_TRIANGLELIST = 4,
		PT_TRIANGLESTRIP = 5
		// Eventually adding more upon need
	};

	enum class RESOURCE_FLAGS : int
	{
		NONE = 0,
		ALLOW_RENDER_TARGET = 0x1,
		ALLOW_DEPTH_STENCIL = 0x2,
		ALLOW_UNORDERED_ACCESS = 0x4,
		DENY_SHADER_RESOURCE = 0x8,
		ALLOW_CROSS_ADAPTER = 0x10,
		ALLOW_SIMULTANEOUS_ACCESS = 0x20,
		VIDEO_DECODE_REFERENCE_ONLY = 0x40
	};
	// Defining bitwise OR operator with RESOURCE_FLAGS so that the enum values can be used as flags
	inline RESOURCE_FLAGS operator | (RESOURCE_FLAGS InLeftValue, RESOURCE_FLAGS InRightValue)
	{
		using T = std::underlying_type<RESOURCE_FLAGS>::type;
		return static_cast<RESOURCE_FLAGS>(static_cast<T>(InLeftValue) | static_cast<T>(InRightValue));
	}
	inline RESOURCE_FLAGS& operator |= (RESOURCE_FLAGS& InLeftValue, RESOURCE_FLAGS InRightValue)
	{
		using T = std::underlying_type<RESOURCE_FLAGS>::type;
		InLeftValue = InLeftValue | InRightValue;
		return InLeftValue;
	}
	inline bool operator & (RESOURCE_FLAGS InLeftValue, RESOURCE_FLAGS InRightValue)
	{
		using T = std::underlying_type<RESOURCE_FLAGS>::type;
		return static_cast<T>(InLeftValue) & static_cast<T>(InRightValue);
	}

enum class COMMAND_LIST_TYPE : int {
	COMMAND_LIST_TYPE_DIRECT = 0,
	COMMAND_LIST_TYPE_BUNDLE = 1,
	COMMAND_LIST_TYPE_COMPUTE = 2,
	COMMAND_LIST_TYPE_COPY = 3,

	UNDEFINED
};

struct CpuDescHandle {
	bool IsBound = false;
};

struct GPUVirtualAddress {
	bool IsValid = false;
};

enum class BUFFER_FORMAT : int {
	R16_UINT, // Single channel 16 bits
	R32G32B32_FLOAT,
	R8G8B8A8_UNORM,
	D32_FLOAT
};

enum class RESOURCE_STATE : int {
	PRESENT = 0,
	RENDER_TARGET = 4
};

struct Resource {
protected:
	Resource() = default;
};

enum class RESOURCE_VIEW_TYPE : int {
	CONSTANT_BUFFER,
	SHADER_RESOURCE,
	UNORDERED_ACCESS
};

struct ResourceView {
protected:
	ResourceView() = default;
	ResourceView(RESOURCE_VIEW_TYPE InType) : m_Type(InType) {}
	RESOURCE_VIEW_TYPE m_Type;
};

struct VertexBufferView { // size is in bytes
	virtual void ReferenceResource(Resource& InResource, size_t InDataSize, size_t InStrideSize) = 0;
};

struct IndexBufferView {
	virtual void ReferenceResource(Resource& InResource, size_t InDataSize, BUFFER_FORMAT InFormat) = 0;
};

struct Rect {
	Rect(int32_t InLeft, int32_t InTop, int32_t InRight, int32_t InBottom) {}
protected:
	Rect(){}
};

struct ViewPort {
	ViewPort(float InTopLeftX, float InTopLeftY, float InWidth, float InHeight) {}
protected:
	ViewPort(){}
};


enum class SHADER_VISIBILITY {
	SV_ALL, SV_VERTEX, SV_HULL, SV_DOMAIN, SV_GEOMETRY, SV_PIXEL
};

struct Shader {
protected:
	Shader() = default;
};


} }

#endif // GraphicsTypes_h__
