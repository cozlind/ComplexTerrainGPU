
cbuffer cbPerObject
{
	float4x4 mWorld;
	float4x4 mWorldInvTranspose;
	float4x4 mWVP;
	float4x4 mTexTransform;
	uint mCornerHeight;
	float worldSpaceVoxelSize;
};
Texture3D noiseTex;
SamplerState Point
{
	Filter = MIN_MAG_MIP_LINEAR;

	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = WRAP;
};

struct vsIn {
	float3 posL : POSITION;
	float2 uv:TEXCOORD;
};

struct vsOut {
	float4 posH:SV_POSITION;
	float3 posW:POSITION;
	float3 uvw:TEXCOORD;
	uint mcCase: TEXCOORD1;
};
struct psIn
{
	float4 posW : SV_POSITION;
	float3 uvw: TEXCOORD;
	//uint instanceID : SV_RenderTargetArrayIndex;
};
vsOut VS(vsIn vin, uint instanceID:SV_InstanceID) {
	vin.posL.y = instanceID;
	vsOut vout;
	vout.posW = mul(float4(vin.posL, 1.0f), mWorld).xyz;
	vout.posH = mul(float4(vin.posL, 1.0f), mWVP);
	float3 uvw = float3(mul(float4(vin.uv, 0.0f, 1.0f), mTexTransform).xy, instanceID / (float)mCornerHeight);
	float2 step = float2(worldSpaceVoxelSize / (float)mCornerHeight, 0);
	float4 f0123 = float4(noiseTex.SampleLevel(Point, uvw + step.yyy, 0).x,
		noiseTex.SampleLevel(Point, uvw + step.yyx, 0).x,
		noiseTex.SampleLevel(Point, uvw + step.xyx, 0).x,
		noiseTex.SampleLevel(Point, uvw + step.xyy, 0).x);
	float4 f4567 = float4(noiseTex.SampleLevel(Point, uvw + step.yxy, 0).x,
		noiseTex.SampleLevel(Point, uvw + step.yxx, 0).x,
		noiseTex.SampleLevel(Point, uvw + step.xxx, 0).x,
		noiseTex.SampleLevel(Point, uvw + step.xxy, 0).x);
	uint4 n0123 = (uint4)saturate(f0123 * 99999);
	uint4 n4567 = (uint4)saturate(f4567 * 99999);
	uint mc_case = (n0123.x) | (n0123.y << 1) | (n0123.z << 2) | (n0123.w << 3)
		| (n4567.x << 4) | (n4567.y << 5) | (n4567.z << 6) | (n4567.w << 7);
	vout.uvw = uvw;
	vout.mcCase = mc_case;
	return vout;
}

float4 PS(vsOut pin) :SV_Target//, uint instanceID : SV_RenderTargetArrayIndex
{
	return pin.mcCase;
	return noiseTex.SampleLevel(Point,pin.uvw,0);
}

//[maxvertexcount(3)]
//void GS(triangle vsOutGsIn input[3], inout TriangleStream<psIn> gsout)
//{
//	psIn pout;
//	for (uint i = 0; i < 3; i++)
//	{
//		output.pos = input[i].position;
//		output.uvw = input[i].uvw;
//		output.instanceID = input[i].instanceID;
//		gsout.Append(output);
//	}
//	gsout.RestartStrip();
//}

//vsOutputGsInput VS(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
//{
//	vsOutputGsInput result;
//	result.uvw = float3(0, 0, 1);
//	result.position = float4(1, 2, 3, 1);
//	result.instanceID = instanceID;
//	return result;
//}
//float4 PS(psInput input) : SV_Target
//{
//	return noiseTex.SampleLevel(LinearRepeat, float3(input.uvw.xy, input.uvw.z / 4.0f),0);
//}
//
technique11 MarchingCubes
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
}