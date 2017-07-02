#include "Table.h"
#include "LightHelper.fx"

#define wsToUvw(ws) (float3(ws.x/160.0f+0.5f, ws.z/160.0f+0.5f,ws.y/160.0f))

cbuffer cbPerFrame
{
	DirectionalLight gDirLights[3];
	float3 gEyePosW;
};
cbuffer cbPerObject
{
	float4x4 mWorld;
	//float4x4 mWorldInvTranspose;
	float4x4 mWVP;
	float4x4 mViewProj;
	float4x4 mTexTransform;
	uint mCornerHeight;//33
	float3 mVoxelSize;
	Material gMaterial;
};
Texture3D noiseTex;
SamplerState Point
{
	Filter = MIN_MAG_MIP_LINEAR;

	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
};

struct vsIn {
	float3 posL : POSITION;
	float2 uv:TEXCOORD;
};

struct vsOutGsIn {
	float3 posW:POSITION1;
	float3 uvw:TEXCOORD;
	float4 f0123:NOISE1;
	float4 f4567:NOISE2;
	uint mcCase: TEXCOORD1;
};
struct psInGsOut
{
	float3 posW : POSITION1;
	float4 posH : SV_POSITION;
	float3 uvw: TEXCOORD;
	float3 normal: NORMAL;
	//uint instanceID : SV_RenderTargetArrayIndex;
};
vsOutGsIn VS(vsIn vin, uint instanceID:SV_InstanceID) {
	vsOutGsIn vout;
	vout.posW = mul(float4(vin.posL, 1.0f), mWorld).xyz;
	vout.posW.y = instanceID*mVoxelSize.y;
	float3 uvw = wsToUvw(vout.posW);
	float2 step = float2(1.0f / 32, 0);
	vout.f0123 = float4(noiseTex.SampleLevel(Point, uvw + step.yyy, 0).x,
		noiseTex.SampleLevel(Point, uvw + step.yyx, 0).x,
		noiseTex.SampleLevel(Point, uvw + step.xyx, 0).x,
		noiseTex.SampleLevel(Point, uvw + step.xyy, 0).x);
	vout.f4567 = float4(noiseTex.SampleLevel(Point, uvw + step.yxy, 0).x,
		noiseTex.SampleLevel(Point, uvw + step.yxx, 0).x,
		noiseTex.SampleLevel(Point, uvw + step.xxx, 0).x,
		noiseTex.SampleLevel(Point, uvw + step.xxy, 0).x);
	uint4 n0123 = (uint4)saturate(vout.f0123 * 99999);
	uint4 n4567 = (uint4)saturate(vout.f4567 * 99999);
	uint mcCase = (n0123.x) | (n0123.y << 1) | (n0123.z << 2) | (n0123.w << 3)
		| (n4567.x << 4) | (n4567.y << 5) | (n4567.z << 6) | (n4567.w << 7);
	vout.uvw = uvw;
	vout.mcCase = mcCase;
	return vout;
}
float3 ComputeNormal(float3 uvw) {
	float4 step = float4(1.0f / 32, 1.0f / 32, 1.0f / 32, 0);
	float3 gradient = float3(
		noiseTex.SampleLevel(Point, uvw + step.xww, 0).x - noiseTex.SampleLevel(Point, uvw - step.xww, 0).x,
		noiseTex.SampleLevel(Point, uvw + step.wwy, 0).x - noiseTex.SampleLevel(Point, uvw - step.wwy, 0).x,
		noiseTex.SampleLevel(Point, uvw + step.wzw, 0).x - noiseTex.SampleLevel(Point, uvw - step.wzw, 0).x);
	return normalize(-gradient);
}

psInGsOut PlaceVertOnEdge(vsOutGsIn input, int edgeNum) {

	// Along this cell edge, where does the density value hit zero?
	float noise0 = dot(cornerAmask0123[edgeNum], input.f0123) + dot(cornerAmask4567[edgeNum], input.f4567);
	float noise1 = dot(cornerBmask0123[edgeNum], input.f0123) + dot(cornerBmask4567[edgeNum], input.f4567);
	
	float t = abs(0 - noise0) < 0.0001f ? 0 : (abs(noise1 - 0) < 0.0001f ? 1 : saturate((0 - noise0) / (noise1 - noise0))); //0..1
	
	float3 posWithinCell = EdgeStart[edgeNum]+ t * EdgeDir[edgeNum]; //[0..1]
	float3 v = input.posW+ posWithinCell*mVoxelSize;
	//float3 uvw = input.uvw + (pos_within_cell*inv_voxelDimMinusOne).xzy;

	//output.wsCoord_Ambo.xyz = wsCoord;
	//output.wsCoord_Ambo.w = grad_ambo_tex.SampleLevel(s, uvw, 0).w;

	psInGsOut output;
	output.posW = v;
	output.uvw = wsToUvw(v);
	output.normal = ComputeNormal(output.uvw);
	output.posH = mul(float4(v, 1.0f), mViewProj);
	return output;
}
[maxvertexcount(15)]
void GS(point vsOutGsIn input[1], inout TriangleStream <psInGsOut> Stream)
{
	psInGsOut output;
	uint num_polys = case_to_numpolys[input[0].mcCase];
	for (uint p = 0; p < num_polys; p++) {
		output = PlaceVertOnEdge(input[0], triTable[input[0].mcCase][p].x);
		Stream.Append(output);
		output = PlaceVertOnEdge(input[0], triTable[input[0].mcCase][p].y);
		Stream.Append(output);
		output = PlaceVertOnEdge(input[0], triTable[input[0].mcCase][p].z);
		Stream.Append(output);
		Stream.RestartStrip();
	}
}
float4 PS(psInGsOut pin, uniform int gLightCount) :SV_Target//, uint instanceID : SV_RenderTargetArrayIndex
{
	////mark the voxel edge
	//if (abs(pin.posW.x / mVoxelSize.x - round(pin.posW.x / mVoxelSize.x)) < 0.01f) return (1,0,0,1);
	//if (abs(pin.posW.y / mVoxelSize.y - round(pin.posW.y / mVoxelSize.y)) < 0.01f) return (1,0,0,1);
	//if (abs(pin.posW.z / mVoxelSize.z - round(pin.posW.z / mVoxelSize.z)) < 0.01f) return (1, 0, 0, 1);

	float3 toEye = gEyePosW - pin.posW;
	float distToEye = length(toEye);
	toEye /= distToEye;

	float4 texColor = float4(1, 1, 1, 1);// = noiseTex.SampleLevel(Point, pin.uvw, 0);
	float4 litColor = texColor;
	if (gLightCount > 0)
	{
		// Start with a sum of zero.
		float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
		float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
		float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

		// Sum the light contribution from each light source.  
		[unroll]
		for (int i = 0; i < gLightCount; ++i)
		{
			float4 A, D, S;
			ComputeDirectionalLight(gMaterial, gDirLights[i], pin.normal, toEye, A, D, S);

			ambient += A;
			diffuse += D;
			spec += S;
		}

		// Modulate with late add.
		litColor = texColor*(ambient + diffuse) + spec;
	}
	return litColor+float4(0.1f,0.1f,0.1f,0.1f)*0.9f;
}
technique11 MarchingCubes
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(CompileShader(gs_5_0, GS()));
		SetPixelShader(CompileShader(ps_5_0, PS(3)));
	}
}