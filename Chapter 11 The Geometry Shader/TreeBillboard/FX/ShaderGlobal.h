#pragma once
cbuffer LodCB{
	float WorldSpaceVolumeHeight = 2.0*(256 / 64.0);
	float3 voxelDim = float3(64, 256, 64);
	float3 voxelDimMinusOne = float3(63, 255, 63);
	float3 wsVoxelSize = 2.0 / 63.0;
	float4 invVoxelDim = float4(1.0 / voxelDim, 0);
	float4 invVoxelDimMinusOne = float4(1.0 / voxelDimMinusOne, 0);
}