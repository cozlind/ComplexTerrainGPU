//***************************************************************************************
// Vertex.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Defines vertex structures and input layouts.
//***************************************************************************************

#ifndef VERTEX_H
#define VERTEX_H

#include "d3dUtil.h"
using namespace DirectX;
using namespace PackedVector;
namespace Vertex
{
	// Basic 32-byte vertex structure.
	struct Basic32
	{
		Basic32() : pos(0.0f, 0.0f, 0.0f), Normal(0.0f, 0.0f, 0.0f), uv(0.0f, 0.0f) {}
		Basic32(const XMFLOAT3& p, const XMFLOAT3& n, const XMFLOAT2& uv)
			: pos(p), Normal(n), uv(uv) {}
		Basic32(float px, float py, float pz, float nx, float ny, float nz, float u, float v)
			: pos(px, py, pz), Normal(nx, ny, nz), uv(u,v) {}
		XMFLOAT3 pos;
		XMFLOAT3 Normal;
		XMFLOAT2 uv;
	};

	struct DensityQuad
	{
		XMFLOAT3 Pos;
	};
	struct TerrainVertex {
		TerrainVertex() :pos(0.0f, 0.0f, 0.0f), uv(0.0f, 0.0f){}	
		TerrainVertex(const XMFLOAT3& p, const XMFLOAT2& t): pos(p),uv(t) {}
		TerrainVertex(float px, float py, float pz,  float u, float v): pos(px, py, pz),uv(u, v) {}
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	};
}

class InputLayoutDesc
{
public:
	// Init like const int A::a[4] = {0, 1, 2, 3}; in .cpp file.
	static const D3D11_INPUT_ELEMENT_DESC Basic32[3];
	static const D3D11_INPUT_ELEMENT_DESC TreePointSprite[2];
	static const D3D11_INPUT_ELEMENT_DESC BuildDensity[1];
	static const D3D11_INPUT_ELEMENT_DESC MarchingCubes[2];
};

class InputLayouts
{
public:
	static void InitAll(ID3D11Device* device);
	static void DestroyAll();

	static ID3D11InputLayout* Basic32;
	static ID3D11InputLayout* TreePointSprite;
	static ID3D11InputLayout* MarchingCubes;
};

#endif // VERTEX_H
