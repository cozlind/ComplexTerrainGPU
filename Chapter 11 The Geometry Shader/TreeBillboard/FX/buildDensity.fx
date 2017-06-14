struct vsOutGsIn {
	float4 position : SV_Position;
	float3 uvw: TexCoord;
	uint instanceID: SliceIndex;
};
struct psIn
{
	float4 pos : SV_POSITION;
	float3 uvw: TEXCOORD0;
	uint instanceID : SV_RenderTargetArrayIndex; //This will write your vertex to a specific slice, which you can read in pixel shader too
};
vsOutGsIn VS(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
	vsOutGsIn result;
	//result.uv = float2((vertexID << 1) & 2, vertexID & 2)*float2(0.5f, 0.5f);//(0,0)(1,0)(0,1)(1,1)
	result.uvw = vertexID % 4 == 0 ? float3(1, 0, instanceID) : float3(0, 0, instanceID);
	result.uvw = vertexID % 4 == 1 ? float3(0, 0, instanceID) : result.uvw;
	result.uvw = vertexID % 4 == 2 ? float3(1, 1, instanceID) : result.uvw;
	result.uvw = vertexID % 4 == 3 ? float3(0, 1, instanceID) : result.uvw;
	//result.position = float4(result.uv * float2(1, 1) , 0.0f, 1.0f);
	float w = instanceID /3.0f;
	result.position = vertexID % 4 == 0 ? float4(0.0f+ w, 0, 0, 1) : float4(-1 + w , 0, 0, 1);
	result.position = vertexID % 4 == 1 ? float4(-1 + w , 0, 0, 1) : result.position;
	result.position = vertexID % 4 == 2 ? float4(0 + w, 1, 0, 1) : result.position;
	result.position = vertexID % 4 == 3 ? float4(-1 + w , 1, 0, 1) : result.position;
	result.instanceID = instanceID;
	return result;
}


[maxvertexcount(3)]
void GS(triangle vsOutGsIn input[3], inout TriangleStream<psIn> gsout)
{
	psIn output;
	for (uint i = 0; i < 3; i++)
	{
		output.pos = input[i].position;
		output.uvw = input[i].uvw;
		output.instanceID = input[i].instanceID;
		gsout.Append(output);
	}
	gsout.RestartStrip();
}

SamplerState Point
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
};
Texture3D noiseTex;
float4 PS(psIn input) : SV_Target
{
	return noiseTex.SampleLevel(Point, float3(input.uvw.xy, input.uvw.z/4.0f),0);
}
technique11 Test1
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(CompileShader(gs_5_0, GS()));
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
}