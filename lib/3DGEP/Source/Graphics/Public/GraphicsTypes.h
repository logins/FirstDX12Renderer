#ifndef GraphicsTypes_h__
#define GraphicsTypes_h__
#include <type_traits>
#include <string>
#include <vector>
namespace GEPUtils { namespace Graphics {

	class CommandList;

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

struct GPU_DESC_HANDLE {
	bool IsBound = false;
};

struct GPUVirtualAddress {
	bool IsValid = false;
};

enum class BUFFER_FORMAT : int {
	R16_UINT, // Single channel 16 bits
	R32G32B32_FLOAT,
	R8G8B8A8_UNORM,
	D32_FLOAT,
	BC1_UNORM
};

enum class TEXTURE_FILE_FORMAT : int {
	DDS,
	PNG,
	JPG
};

enum class RESOURCE_HEAP_TYPE : int {
	DEFAULT = 0,
	UPLOAD = 1
};

enum class RESOURCE_STATE : int {
	PRESENT = 0,
	RENDER_TARGET,
	COPY_SOURCE,
	COPY_DEST,
	GEN_READ
};

enum class TEXTURE_TYPE : int {
	TEX_1D,
	TEX_2D,
	TEX_CUBE,
	TEX_3D
};

struct Resource {
	size_t GetDataSize() const { return m_DataSize; }
	size_t GetAlignSize() const { return m_AlignmentSize; }
protected:
	Resource() : m_DataSize(0), m_AlignmentSize(0) {};
	size_t m_DataSize;
	size_t m_AlignmentSize;
};

struct Buffer : public Resource {
};

struct Texture : public Resource {

	virtual void UploadToGPU(GEPUtils::Graphics::CommandList& InCommandList, GEPUtils::Graphics::Buffer& InIntermediateBuffer) = 0;

	virtual size_t GetGPUSize() = 0;

	BUFFER_FORMAT GetFormat() { return m_TexelFormat; }
	TEXTURE_TYPE GetType() { return m_Type; }
	size_t GetMipLevelsNum() { return m_MipLevelsNum; }

protected:
	size_t          m_Width;
	size_t          m_Height;     // Should be 1 for 1D textures
	size_t          m_ArraySize;  // For cubemap, this is a multiple of 6
	size_t          m_MipLevelsNum;
	BUFFER_FORMAT   m_TexelFormat;
	TEXTURE_TYPE    m_Type;
	std::vector<unsigned char> m_Data;
};

struct DynamicBuffer : public Resource {
	virtual void SetData(void* InData, size_t InSize, size_t InAlignmentSize) = 0;

	void* GetData() { return m_Data.data(); }
	// Note: BufferSize indicates the whole container while DataSize only the effective data stored in the buffer 
	size_t GetBufferSize() const { return m_BufferSize; }
protected:
	size_t m_BufferSize;
	std::vector<unsigned char> m_Data;
};

enum class RESOURCE_VIEW_TYPE : int {
	CONSTANT_BUFFER,
	SHADER_RESOURCE,
	UNORDERED_ACCESS
};

struct ResourceView {

};

struct ConstantBufferView : public ResourceView {
	virtual void ReferenceBuffer(Buffer& InResource, size_t InDataSize, size_t InStrideSize) = 0;
protected:
	ConstantBufferView() = default;
};

struct ShaderResourceView : public ResourceView {
	virtual void ReferenceTexture(GEPUtils::Graphics::Texture& InTexture) = 0;
protected:
	ShaderResourceView() = default;
};

struct VertexBufferView { // size is in bytes
	virtual void ReferenceResource(GEPUtils::Graphics::Resource& InResource, size_t DataSize, size_t StrideSize) = 0;
protected:
	VertexBufferView() = default;
};

struct IndexBufferView {
	virtual void ReferenceResource(GEPUtils::Graphics::Resource& InResource, size_t InDataSize, BUFFER_FORMAT InFormat) = 0;
protected:
	IndexBufferView() = default;
};

enum class SAMPLE_FILTER_TYPE : int {
	POINT,
	LINEAR,
	ANISOTROPIC
};

enum class TEXTURE_ADDRESS_MODE : int {
	WRAP,
	MIRROR,
	CLAMP,
	BORDER
};

struct StaticSampler {
	StaticSampler(uint32_t InShaderRegister, SAMPLE_FILTER_TYPE InFilterType)
		: m_ShaderRegister(InShaderRegister), m_Filter(InFilterType), 
		m_AddressU(TEXTURE_ADDRESS_MODE::WRAP),	m_AddressV(TEXTURE_ADDRESS_MODE::WRAP),	m_AddressW(TEXTURE_ADDRESS_MODE::WRAP)
	{ }
	uint32_t m_ShaderRegister;
	SAMPLE_FILTER_TYPE m_Filter;
	TEXTURE_ADDRESS_MODE m_AddressU;
	TEXTURE_ADDRESS_MODE m_AddressV;
	TEXTURE_ADDRESS_MODE m_AddressW;
};

struct Rect {
	Rect(int32_t InLeft, int32_t InTop, int32_t InRight, int32_t InBottom) {}
protected:
	Rect(){}
};

struct ViewPort {
	ViewPort(float InTopLeftX, float InTopLeftY, float InWidth, float InHeight) {}
	virtual void SetWidthAndHeight(float InWidth, float InHeight) {}
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
