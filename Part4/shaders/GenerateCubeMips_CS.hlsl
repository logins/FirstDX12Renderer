

#define BLOCK_SIZE 8
#define BLOCK_HANDLED_TEXELS BLOCK_SIZE*BLOCK_SIZE

struct ComputeShaderInput {
    uint3 GroupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
    uint3 GroupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
    uint3 DispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
    uint  GroupIndex        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};

cbuffer GenerateMipsCB : register(b0)
{
    float2 Mip1Size;
}

// Source mip
Texture2DArray<float4> SrcMip : register(t0);

// Output mips
RWTexture2DArray<float4> OutMip1 : register(u0);
RWTexture2DArray<float4> OutMip2 : register(u1);
RWTexture2DArray<float4> OutMip3 : register(u2);
RWTexture2DArray<float4> OutMip4 : register(u3);

SamplerState Mip0Sampler : register(s0);

// Local Data Share (LDS) memory, shared among threads of the Same thread group,
// that we are going to use to generate the next mip from previous level computation.
// The size is 64 because each thread group is handling 8x8 texels.
// We are using 4 different arrays of size 1 to reduce bank conflicts in the local data memory controller.
// A large stri
groupshared float gs_R[BLOCK_HANDLED_TEXELS];
groupshared float gs_G[BLOCK_HANDLED_TEXELS];
groupshared float gs_B[BLOCK_HANDLED_TEXELS];
groupshared float gs_A[BLOCK_HANDLED_TEXELS];

void StoreColor(uint InIdx, float4 InColor)
{
    gs_R[InIdx] = InColor.r;
    gs_G[InIdx] = InColor.g;
    gs_B[InIdx] = InColor.b;
    gs_A[InIdx] = InColor.a;
}

float4 LoadColor(uint InIdx)
{
    return float4(gs_R[InIdx], gs_G[InIdx], gs_B[InIdx], gs_A[InIdx]);
}

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)] // Each thread group is made by 8 by 8 threads, handling a 8x8 texel portion of a single mip chain
void main( ComputeShaderInput IN )
{
	// Mip1 computation
    float3 mip1Coords;
    mip1Coords.xy = (IN.DispatchThreadID.xy + 0.5) / Mip1Size;
    mip1Coords.z = IN.DispatchThreadID.z; // The third thread coordinate selects what face to compute

    float4 texel1 = SrcMip.SampleLevel(Mip0Sampler, mip1Coords, 0);

    OutMip1[IN.DispatchThreadID] = texel1;

    StoreColor(IN.GroupIndex, texel1);

    // We use GroupMemoryBarrierWithGroupSync() system call to synchronize all the operations within a thread group
    // so when we get past this function, we can be sure that all the operations of the thread group up to this point have been completed.
    // We then are going to be able to use all the computed values in our groupshared LDS memory.
    GroupMemoryBarrierWithGroupSync();

    // Mip2 computation
    if ((IN.GroupIndex & 0x9) == 0) // If the current flattened thread group index on X and Y is multiple of 2.
    // This is possible because X and Y range from 0 to 7, and so in binary from 0 to 111.
    // To check if the first bit is 0 for X and Y, and so that be a multiple of 2, we just need an And operation with 001001 = 0x9 to be 0.
    {
        // We now get the texels down, right and down-right of the previously computed texel1 and we average them (like a bilinear sample)
        // to get the texel value of mip2.
        float4 texel2 = LoadColor(IN.GroupIndex + 0x01); // Right (+000001)
        float4 texel3 = LoadColor(IN.GroupIndex + 0x08); // Down (+001000)
        float4 texel4 = LoadColor(IN.GroupIndex + 0x09); // Down-Right (+001001)

        texel1 = 0.25 * (texel1 + texel2 + texel3 + texel4);

        OutMip2[int3(IN.DispatchThreadID.xy / 2, IN.DispatchThreadID.z)] = texel1;

        // Note: We are always modifying texel1 value and store it again in shared memory.
        // This is because we are using the new value to compute the next mip levels!
        // We don't have to worry about the previous mip, because it has already been stored 
        // and so the shared memory is ready to be reused at our will!
        StoreColor(IN.GroupIndex, texel1);
    }

    GroupMemoryBarrierWithGroupSync(); // Ensure operations on mip2 for the current thread group have been executed

    // Mip3 computation
    if ((IN.GroupIndex & 0x1B) == 0) // X and Y both multiple of 4 (in And with 011011 = 0x1B)
    {
        // For Mip3 we need to retrieve the texels computed by mip2, and so the shift is going to be of 2 positions
        float4 texel2 = LoadColor(IN.GroupIndex + 0x02); // Right (+000010)
        float4 texel3 = LoadColor(IN.GroupIndex + 0x10); // Down (+010000)
        float4 texel4 = LoadColor(IN.GroupIndex + 0x12); // Down-Right (+010010)

        texel1 = 0.25 * (texel1 + texel2 + texel3 + texel4);

        OutMip3[int3(IN.DispatchThreadID.xy / 4, IN.DispatchThreadID.z)] = texel1;

        StoreColor(IN.GroupIndex, texel1);
    }

    GroupMemoryBarrierWithGroupSync(); // Ensure operations on mip3 for the current thread group have been executed

    // Mip4 computation
    // For Mip4 we would have to check if the current coordinates are multiple of 8, and so in And with 111111
    // but we only have two pairs of coordinates that match this condition: 0,0 and 7,7.
    // Still, we are interested to compute only 1 texel to match this condition, the one at (0,0), because it is the only one that
    // we can compute in the current thread group with the local texel mip informations that we have.
    if (IN.GroupIndex == 0)
    {
        // Retrieve texels computed for mip3, which shift of 4 positions from the current texel
        float4 texel2 = LoadColor(IN.GroupIndex + 0x04); // Right (+000100)
        float4 texel3 = LoadColor(IN.GroupIndex + 0x20); // Down (+100000)
        float4 texel4 = LoadColor(IN.GroupIndex + 0x24); // Down-Right (+100100)

        texel1 = 0.25 * (texel1 + texel2 + texel3 + texel4);

        OutMip4[int3(IN.DispatchThreadID.xy / 8, IN.DispatchThreadID.z)] = texel1;
    }

}


