
cbuffer cbPerObject
{
	float4x4 mWorld;
	float4x4 mWorldInvTranspose;
	float4x4 mWVP;
	float4x4 mTexTransform;
};
Texture3D noiseTex;
SamplerState Point
{
	Filter = MIN_MAG_MIP_POINT;

	AddressU = WRAP;
	AddressV = WRAP;
	AddressW = WRAP;
};

struct vsIn {
	float3 posL : POSITION;
	float2 uv:TEXCOORD;
};

struct vsOut {
	float4 posH:SV_POSITION;
	float3 posW:POSITION;
	float2 uv:TEXCOORD;
	uint instanceID:sliceIndex;
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
	vout.uv = mul(float4(vin.uv, 0.0f, 1.0f), mTexTransform).xy;
	vout.instanceID = instanceID;
	return vout;
}

float4 PS(vsOut pin) :SV_Target//, uint instanceID : SV_RenderTargetArrayIndex
{ 
	return noiseTex.SampleLevel(Point, float3(pin.uv, pin.instanceID / 4.0f),0);

	return float4(0.5f, 1.0f, 0.8f,1.0f);
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