//***************************************************************************************
// TreeBillboardDemo.cpp by Frank Luna (C) 2011 All Rights Reserved.
//
// Demonstrates the geometry shader, texture arrays, and alpha to coverage.
//
// Controls:
//		Hold the left mouse button down and move the mouse to rotate.
//      Hold the right mouse button down to zoom in and out.
//
//      Press '1' - Lighting only render mode.
//      Press '2' - Texture render mode.
//      Press '3' - Fog render mode.
//      Press 'r' - Alpha-to-coverage on.
//      Press 't' - Alpha-to-coverage off.
//
//***************************************************************************************

#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "Effects.h"
#include "Vertex.h"
#include "RenderStates.h"
#include "Waves.h"
#include <string>
#include "FastNoise.h"

const int cornerWidth = 33;
const int cornerDepth = 33;
const int cornerHeight = 4;
const int voxelWidth = cornerWidth - 1;
const int voxelDepth = cornerDepth - 1;
const int voxelHeight = cornerHeight - 1;


enum RenderOptions
{
	Lighting = 0,
	Textures = 1,
	TexturesAndFog = 2
};

class TerrainApp : public D3DApp
{
public:
	TerrainApp(HINSTANCE hInstance);
	~TerrainApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	float GetHillHeight(float x, float z)const;
	XMFLOAT3 GetHillNormal(float x, float z)const;
	void BuildLandGeometryBuffers();
	void BuildWaveGeometryBuffers();
	void BuildCrateGeometryBuffers();
	void InitDensitySRV();
	void BuildTerrainGeometryBuffers();
	void DrawDensityFX(CXMMATRIX viewProj);
	void HandleImGui();

private:
	ID3D11Buffer* mLandVB;
	ID3D11Buffer* mLandIB;

	ID3D11Buffer* mWavesVB;
	ID3D11Buffer* mWavesIB;

	ID3D11Buffer* mBoxVB;
	ID3D11Buffer* mBoxIB;

	ID3D11Buffer* mTerrainVB;
	ID3D11Buffer* mTerrainIB;

	ID3D11ShaderResourceView* mGrassMapSRV;
	ID3D11ShaderResourceView* mWavesMapSRV;
	ID3D11ShaderResourceView* mBoxMapSRV;
	ID3D11ShaderResourceView* mDensitySRV;

	Waves mWaves;

	DirectionalLight mDirLights[3];
	Material mLandMat;
	Material mWavesMat;
	Material mBoxMat;
	Material mTreeMat;

	XMFLOAT4X4 mGrassTexTransform;
	XMFLOAT4X4 mWaterTexTransform;
	XMFLOAT4X4 mTerrainTexTransform;
	XMFLOAT4X4 mLandWorld;
	XMFLOAT4X4 mWavesWorld;
	XMFLOAT4X4 mBoxWorld;

	XMFLOAT4X4 mTerrainWorld;

	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	UINT mLandIndexCount;
	UINT mTerrainIndexCount;

	static const UINT NoiseQuadPointCount = 28;

	bool mAlphaToCoverageOn;

	XMFLOAT2 mWaterTexOffset;

	RenderOptions mRenderOptions;

	XMFLOAT3 mEyePosW;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;

	ID3D11Texture3D* mDensityTexture3d;
	ID3D11RenderTargetView* mDensityRTV;


	float noiseMap[cornerWidth * cornerDepth * cornerHeight];

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	TerrainApp theApp(hInstance);

	if (!theApp.Init())
		return 0;

	return theApp.Run();
}

TerrainApp::TerrainApp(HINSTANCE hInstance)
	: D3DApp(hInstance), mLandVB(0), mLandIB(0), mWavesVB(0), mWavesIB(0), mBoxVB(0), mBoxIB(0), mTerrainVB(0), mTerrainIB(0),
	mGrassMapSRV(0), mWavesMapSRV(0), mBoxMapSRV(0), mAlphaToCoverageOn(true),
	mWaterTexOffset(0.0f, 0.0f), mEyePosW(0.0f, 0.0f, 0.0f), mLandIndexCount(0), mTerrainIndexCount(0), mRenderOptions(RenderOptions::TexturesAndFog),
	mTheta(1.3f*MathHelper::Pi), mPhi(0.4f*MathHelper::Pi), mRadius(80.0f)
{
	mMainWndCaption = L"Terrain Demo";
	mEnable4xMsaa = true;

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mLandWorld, I);
	XMStoreFloat4x4(&mTerrainWorld, I);
	XMStoreFloat4x4(&mWavesWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);
	XMStoreFloat4x4(&mTerrainTexTransform, I);

	XMMATRIX boxScale = XMMatrixScaling(15.0f, 15.0f, 15.0f);
	XMMATRIX boxOffset = XMMatrixTranslation(8.0f, 5.0f, -15.0f);
	XMStoreFloat4x4(&mBoxWorld, boxScale*boxOffset);

	XMMATRIX grassTexScale = XMMatrixScaling(5.0f, 5.0f, 0.0f);
	XMStoreFloat4x4(&mGrassTexTransform, grassTexScale);

	mDirLights[0].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[0].Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLights[0].Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLights[0].Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	mDirLights[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[1].Diffuse = XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
	mDirLights[1].Specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	mDirLights[1].Direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	mDirLights[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[2].Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Direction = XMFLOAT3(0.0f, -0.707f, -0.707f);

	mLandMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mLandMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mLandMat.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	mWavesMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mWavesMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	mWavesMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);

	mBoxMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mBoxMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mBoxMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	mTreeMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mTreeMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mTreeMat.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
}

TerrainApp::~TerrainApp()
{
	md3dImmediateContext->ClearState();
	ReleaseCOM(mLandVB);
	ReleaseCOM(mLandIB);
	ReleaseCOM(mWavesVB);
	ReleaseCOM(mWavesIB);
	ReleaseCOM(mBoxVB);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mTerrainVB);
	ReleaseCOM(mTerrainIB);
	ReleaseCOM(mGrassMapSRV);
	ReleaseCOM(mWavesMapSRV);
	ReleaseCOM(mBoxMapSRV);
	ReleaseCOM(mDensityRTV);
	ReleaseCOM(mDensitySRV);
	ReleaseCOM(mDensityTexture3d);

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}

bool TerrainApp::Init()
{
	if (!D3DApp::Init())
		return false;


	mWaves.Init(160, 160, 1.0f, 0.03f, 5.0f, 0.3f);

	// Must init Effects first since InputLayouts depend on shader signatures.
	Effects::InitAll(md3dDevice);
	InputLayouts::InitAll(md3dDevice);
	RenderStates::InitAll(md3dDevice);
	ID3D11Resource* texResource = nullptr;
	HR(DirectX::CreateDDSTextureFromFile(md3dDevice,
		L"Textures/grass.dds", &texResource, &mGrassMapSRV));
	ReleaseCOM(texResource); // view saves reference

	HR(DirectX::CreateDDSTextureFromFile(md3dDevice,
		L"Textures/water2.dds", &texResource, &mWavesMapSRV));
	ReleaseCOM(texResource); // view saves reference

	HR(DirectX::CreateDDSTextureFromFile(md3dDevice,
		L"Textures/WireFence.dds", &texResource, &mBoxMapSRV));
	ReleaseCOM(texResource); // view saves reference

	HR(DirectX::CreateDDSTextureFromFile(md3dDevice,
		L"Textures/volTex.dds", &texResource, &mDensitySRV));
	ReleaseCOM(texResource); // view saves reference


	InitDensitySRV();


	BuildLandGeometryBuffers();
	BuildWaveGeometryBuffers();
	BuildCrateGeometryBuffers();
	BuildTerrainGeometryBuffers();

	return true;
}

void TerrainApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void TerrainApp::UpdateScene(float dt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius*sinf(mPhi)*cosf(mTheta);
	float z = mRadius*sinf(mPhi)*sinf(mTheta);
	float y = mRadius*cosf(mPhi);

	mEyePosW = XMFLOAT3(x, y, z);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);

	//
	// Every quarter second, generate a random wave.
	//
	static float t_base = 0.0f;
	if ((mTimer.TotalTime() - t_base) >= 0.1f)
	{
		t_base += 0.1f;

		DWORD i = 5 + rand() % (mWaves.RowCount() - 10);
		DWORD j = 5 + rand() % (mWaves.ColumnCount() - 10);

		float r = MathHelper::RandF(0.5f, 1.0f);

		mWaves.Disturb(i, j, r);
	}

	mWaves.Update(dt);

	//
	// Update the wave vertex buffer with the new solution.
	//

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(md3dImmediateContext->Map(mWavesVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

	Vertex::Basic32* v = reinterpret_cast<Vertex::Basic32*>(mappedData.pData);
	for (UINT i = 0; i < mWaves.VertexCount(); ++i)
	{
		v[i].pos = mWaves[i];
		//v[i].Normal = mWaves.Normal(i);

		// Derive tex-coords in [0,1] from position.
		v[i].uv.x = 0.5f + mWaves[i].x / mWaves.Width();
		v[i].uv.y = 0.5f - mWaves[i].z / mWaves.Depth();
	}

	md3dImmediateContext->Unmap(mWavesVB, 0);

	//
	// Animate water texture coordinates.
	//

	// Tile water texture.
	XMMATRIX wavesScale = XMMatrixScaling(5.0f, 5.0f, 0.0f);

	// Translate texture over time.
	mWaterTexOffset.y += 0.05f*dt;
	mWaterTexOffset.x += 0.1f*dt;
	XMMATRIX wavesOffset = XMMatrixTranslation(mWaterTexOffset.x, mWaterTexOffset.y, 0.0f);

	// Combine scale and translation.
	XMStoreFloat4x4(&mWaterTexTransform, wavesScale*wavesOffset);

	//
	// Switch the render mode based in key input.
	//
	if (GetAsyncKeyState('1') & 0x8000)
		mRenderOptions = RenderOptions::Lighting;

	if (GetAsyncKeyState('2') & 0x8000)
		mRenderOptions = RenderOptions::Textures;

	if (GetAsyncKeyState('3') & 0x8000)
		mRenderOptions = RenderOptions::TexturesAndFog;

	if (GetAsyncKeyState('R') & 0x8000)
		mAlphaToCoverageOn = true;

	if (GetAsyncKeyState('T') & 0x8000)
		mAlphaToCoverageOn = false;
}

void TerrainApp::DrawScene()
{
	//gui
	HandleImGui();
	//background
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	//basic reference
	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX viewProj = view*proj;



	md3dImmediateContext->IASetInputLayout(InputLayouts::MarchingCubes);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	UINT stride = sizeof(Vertex::TerrainVertex);
	UINT offset = 0;

	//begin to draw
	D3DX11_TECHNIQUE_DESC techDesc;
	Effects::MarchingCubesFX->MarchingCubes->GetDesc(&techDesc);

	//
	// Draw the hills.
	//
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mTerrainVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mTerrainIB, DXGI_FORMAT_R32_UINT, 0);

	// Set per object constants.
	XMMATRIX world = XMLoadFloat4x4(&mTerrainWorld);
	XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
	XMMATRIX worldViewProj = world*view*proj;

	Effects::MarchingCubesFX->SetWorld(world);
	Effects::MarchingCubesFX->SetWorldInvTranspose(worldInvTranspose);
	Effects::MarchingCubesFX->SetWorldViewProj(worldViewProj);
	Effects::MarchingCubesFX->SetTexTransform(XMLoadFloat4x4(&mTerrainTexTransform));
	Effects::MarchingCubesFX->SetNoiseTex(mDensitySRV);
	Effects::MarchingCubesFX->MarchingCubes->GetPassByIndex(0)->Apply(0, md3dImmediateContext);

	md3dImmediateContext->DrawIndexedInstanced(mTerrainIndexCount, voxelHeight, 0, 0,0);

	//md3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);

	//
	// Draw the density texture3d
	//

	//DrawDensityFX(viewProj);


#pragma region drawSinLand  
	//md3dImmediateContext->IASetInputLayout(InputLayouts::Basic32);
	//md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//stride = sizeof(Vertex::Basic32);
	//offset = 0;

	////
	//// Set per frame constants for the rest of the objects.
	////
	//Effects::BasicFX->SetDirLights(mDirLights);
	//Effects::BasicFX->SetEyePosW(mEyePosW);
	//Effects::BasicFX->SetFogColor(Colors::Silver);
	//Effects::BasicFX->SetFogStart(15.0f);
	//Effects::BasicFX->SetFogRange(175.0f);

	////
	//// Figure out which technique to use.
	////
	//ID3DX11EffectTechnique* boxTech;
	//ID3DX11EffectTechnique* landAndWavesTech;

	//switch (mRenderOptions)
	//{
	//case RenderOptions::Lighting:
	//	boxTech = Effects::BasicFX->Light3Tech;
	//	landAndWavesTech = Effects::BasicFX->Light3Tech;
	//	break;
	//case RenderOptions::Textures:
	//	boxTech = Effects::BasicFX->Light3TexAlphaClipTech;
	//	landAndWavesTech = Effects::BasicFX->Light3TexTech;
	//	break;
	//case RenderOptions::TexturesAndFog:
	//	boxTech = Effects::BasicFX->Light3TexAlphaClipFogTech;
	//	landAndWavesTech = Effects::BasicFX->Light3TexFogTech;
	//	break;
	//}


	////
	//// Draw the box.
	//// 

	////boxTech->GetDesc(&techDesc);
	/////*for (UINT p = 0; p < techDesc.Passes; ++p)
	////{*/
	////UINT p = 0;
	////	md3dImmediateContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);
	////	md3dImmediateContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);

	////	// Set per object constants.
	////	XMMATRIX world = XMLoadFloat4x4(&mBoxWorld);
	////	XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
	////	XMMATRIX worldViewProj = world*view*proj;

	////	Effects::BasicFX->SetWorld(world);
	////	Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
	////	Effects::BasicFX->SetWorldViewProj(worldViewProj);
	////	Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
	////	//Effects::BasicFX->SetMaterial(mBoxMat);
	////	//Effects::BasicFX->SetDiffuseMap(mBoxMapSRV);

	////	//md3dImmediateContext->OMSetBlendState(RenderStates::AlphaToCoverageBS, blendFactor, 0xffffffff);
	////	md3dImmediateContext->RSSetState(RenderStates::NoCullRS);
	////	boxTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
	////	md3dImmediateContext->DrawIndexed(36, 0, 0);

	////	// Restore default render state.
	////	md3dImmediateContext->RSSetState(0);
	//////}

	////
	//// Draw the hills and water with texture and fog (no alpha clipping needed).
	////

	//landAndWavesTech->GetDesc(&techDesc);
	//UINT p = 0;
	//	
	//	md3dImmediateContext->IASetVertexBuffers(0, 1, &mLandVB, &stride, &offset);
	//	md3dImmediateContext->IASetIndexBuffer(mLandIB, DXGI_FORMAT_R32_UINT, 0);

	//	world = XMLoadFloat4x4(&mLandWorld);
	//	worldInvTranspose = MathHelper::InverseTranspose(world);
	//	worldViewProj = world*view*proj;

	//	Effects::BasicFX->SetWorld(world);
	//	Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
	//	Effects::BasicFX->SetWorldViewProj(worldViewProj);
	//	Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&mGrassTexTransform));
	//	Effects::BasicFX->SetMaterial(mLandMat);
	//	Effects::BasicFX->SetDiffuseMap(mGrassMapSRV);

	//	landAndWavesTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
	//	md3dImmediateContext->DrawInstanced(mLandIndexCount, 1,0, 0);

	////}
	//	//
	//	// Draw the waves.
	//	//
	//	md3dImmediateContext->IASetVertexBuffers(0, 1, &mWavesVB, &stride, &offset);
	//	md3dImmediateContext->IASetIndexBuffer(mWavesIB, DXGI_FORMAT_R32_UINT, 0);

	//	// Set per object constants.
	//	world = XMLoadFloat4x4(&mWavesWorld);
	//	worldInvTranspose = MathHelper::InverseTranspose(world);
	//	worldViewProj = world*view*proj;

	//	Effects::BasicFX->SetWorld(world);
	//	Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
	//	Effects::BasicFX->SetWorldViewProj(worldViewProj);
	//	Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&mWaterTexTransform));
	//	Effects::BasicFX->SetMaterial(mWavesMat);
	//	Effects::BasicFX->SetDiffuseMap(mWavesMapSRV);

	//	md3dImmediateContext->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);
	//	landAndWavesTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
	//	md3dImmediateContext->DrawIndexed(3 * mWaves.TriangleCount(), 0, 0);

	//	// Restore default blend state
	//md3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
	//}

#pragma endregion drawSinLand  
 	ImGui::Render();
	HR(mSwapChain->Present(0, 0));
}

void TerrainApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void TerrainApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void TerrainApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.01 unit in the scene.
		float dx = 0.1f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.1f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 20.0f, 500.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

float TerrainApp::GetHillHeight(float x, float z)const
{
	return 0.3f*(z*sinf(0.1f*x) + x*cosf(0.1f*z));
}

XMFLOAT3 TerrainApp::GetHillNormal(float x, float z)const
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
		1.0f,
		-0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}

void TerrainApp::BuildLandGeometryBuffers()
{
	GeometryGenerator::MeshData grid;

	GeometryGenerator geoGen;

	geoGen.CreateGrid(160.0f, 160.0f, 50, 50, grid);

	mLandIndexCount = grid.Indices.size();

	//
	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  
	//

	std::vector<Vertex::Basic32> vertices(grid.Vertices.size());
	for (UINT i = 0; i < grid.Vertices.size(); ++i)
	{
		XMFLOAT3 p = grid.Vertices[i].Position;

		//p.y = GetHillHeight(p.x, p.z);

		vertices[i].pos = p;
		//vertices[i].Normal = GetHillNormal(p.x, p.z);
		vertices[i].uv = grid.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * grid.Vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mLandVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * mLandIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &grid.Indices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mLandIB));
}

void TerrainApp::BuildWaveGeometryBuffers()
{
	// Create the vertex buffer.  Note that we allocate space only, as
	// we will be updating the data every time step of the simulation.

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * mWaves.VertexCount();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbd.MiscFlags = 0;
	HR(md3dDevice->CreateBuffer(&vbd, 0, &mWavesVB));


	// Create the index buffer.  The index buffer is fixed, so we only 
	// need to create and set once.

	std::vector<UINT> indices(3 * mWaves.TriangleCount()); // 3 indices per face

	// Iterate over each quad.
	UINT m = mWaves.RowCount();
	UINT n = mWaves.ColumnCount();
	int k = 0;
	for (UINT i = 0; i < m - 1; ++i)
	{
		for (DWORD j = 0; j < n - 1; ++j)
		{
			indices[k] = i*n + j;
			indices[k + 1] = i*n + j + 1;
			indices[k + 2] = (i + 1)*n + j;

			indices[k + 3] = (i + 1)*n + j;
			indices[k + 4] = i*n + j + 1;
			indices[k + 5] = (i + 1)*n + j + 1;

			k += 6; // next quad
		}
	}

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mWavesIB));
}

void TerrainApp::BuildCrateGeometryBuffers()
{
	GeometryGenerator::MeshData box;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<Vertex::Basic32> vertices(box.Vertices.size());

	for (UINT i = 0; i < box.Vertices.size(); ++i)
	{
		vertices[i].pos = box.Vertices[i].Position;
		//vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].uv = box.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * box.Vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * box.Indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &box.Indices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
}

void TerrainApp::InitDensitySRV()
{

	FastNoise myNoise; // Create a FastNoise object
	myNoise.SetSeed(24);
	myNoise.SetFrequency(0.2f);
	myNoise.SetNoiseType(FastNoise::SimplexFractal); // Set the desired noise type
	float noiseScale = 1;
	for (int y = 0; y < cornerHeight; y++)
	{
		for (int z = 0; z < cornerDepth; z++)
		{
			for (int x = 0; x < cornerWidth; x++)
			{
				noiseMap[y*cornerDepth*cornerWidth + z * cornerWidth + x] = (myNoise.GetNoise(x*noiseScale, z*noiseScale, y*noiseScale) + 1.0f)*0.5f;

			}
		}
	}

	D3D11_SUBRESOURCE_DATA noiseData;
	noiseData.pSysMem = noiseMap;
	noiseData.SysMemPitch = sizeof(float)*cornerWidth;
	noiseData.SysMemSlicePitch = sizeof(float)*cornerWidth*cornerDepth;

	//init texture3d and rendertargetview for render to texture
	D3D11_TEXTURE3D_DESC textureDesc;
	textureDesc.Width = cornerWidth;
	textureDesc.Height = cornerDepth;
	textureDesc.Depth = cornerHeight;
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	HR(md3dDevice->CreateTexture3D(&textureDesc, &noiseData, &mDensityTexture3d));

	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
	renderTargetViewDesc.Texture3D.FirstWSlice = 0;
	renderTargetViewDesc.Texture3D.MipSlice = 0;
	renderTargetViewDesc.Texture3D.WSize = textureDesc.Depth;
	HR(md3dDevice->CreateRenderTargetView(mDensityTexture3d, &renderTargetViewDesc, &mDensityRTV));

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	shaderResourceViewDesc.Texture3D.MipLevels = textureDesc.MipLevels;
	shaderResourceViewDesc.Texture3D.MostDetailedMip = 0;
	HR(md3dDevice->CreateShaderResourceView(mDensityTexture3d, &shaderResourceViewDesc, &mDensitySRV));


}

void TerrainApp::BuildTerrainGeometryBuffers()
{
	GeometryGenerator::MeshData grid;

	GeometryGenerator geoGen;

	geoGen.CreateGrid(160.0f, 160.0f, voxelWidth, voxelDepth, grid);


	mTerrainIndexCount = grid.Indices.size();
	std::vector<Vertex::TerrainVertex> vertices(grid.Vertices.size());
	for (UINT i = 0; i < grid.Vertices.size(); ++i)
	{
		vertices[i].pos = grid.Vertices[i].Position;
		vertices[i].uv = grid.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::TerrainVertex) * grid.Vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mTerrainVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) *  grid.Indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &grid.Indices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mTerrainIB));

}
void TerrainApp::DrawDensityFX(CXMMATRIX viewProj)
{
	/*Effects::TreeSpriteFX->SetDirLights(mDirLights);
	Effects::TreeSpriteFX->SetEyePosW(mEyePosW);
	Effects::TreeSpriteFX->SetFogColor(Colors::Silver);
	Effects::TreeSpriteFX->SetFogStart(15.0f);
	Effects::TreeSpriteFX->SetFogRange(175.0f);
	Effects::TreeSpriteFX->SetViewProj(viewProj);
	Effects::TreeSpriteFX->SetMaterial(mTreeMat);
	Effects::TreeSpriteFX->SetTreeTextureMapArray(mTreeTextureMapArraySRV);*/

	//md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	//md3dImmediateContext->IASetInputLayout(InputLayouts::TreePointSprite);
	//UINT stride = sizeof(Vertex::TreePointSprite);
 //   UINT offset = 0; 

	//ID3DX11EffectTechnique* treeTech;
	//switch(mRenderOptions)
	//{
	//case RenderOptions::Lighting:
	//	treeTech = Effects::TreeSpriteFX->Light3Tech;
	//	break;
	//case RenderOptions::Textures:
	//	treeTech = Effects::TreeSpriteFX->Light3TexAlphaClipTech;
	//	break;
	//case RenderOptions::TexturesAndFog:
	//	treeTech = Effects::TreeSpriteFX->Light3TexAlphaClipFogTech;
	//	break;  
	//}

	//D3DX11_TECHNIQUE_DESC techDesc;
	//treeTech->GetDesc( &techDesc );
	//for(UINT p = 0; p < techDesc.Passes; ++p)
 //   {
	//md3dImmediateContext->IASetVertexBuffers(0, 1, &mTreeSpritesVB, &stride, &offset);
	//	float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	//	if(mAlphaToCoverageOn)
	//	{
			//md3dImmediateContext->OMSetBlendState(RenderStates::AlphaToCoverageBS, blendFactor, 0xffffffff);
	//	}
	//	treeTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
	//	md3dImmediateContext->Draw(TreeCount, 0);
	/*float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	md3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);*/
	//	md3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
	//}


	/*md3dImmediateContext->OMSetRenderTargets(1, &mDensityRTV, mDepthStencilView);
	md3dImmediateContext->ClearRenderTargetView(mDensityRTV, reinterpret_cast<const float*>(&Colors::Silver));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);*/


	Effects::BuildDensityFX->SetNoiseTex(mDensitySRV);

	//draw the 3d texture
	UINT offset = 0;
	UINT stride = 0;
	md3dImmediateContext->IASetInputLayout(NULL);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ID3DX11EffectTechnique * pTech = NULL;
	Effects::BuildDensityFX->Test->GetPassByIndex(0)->Apply(0, md3dImmediateContext);
	md3dImmediateContext->DrawInstanced(4, cornerHeight, 0, 0);


}
void TerrainApp::HandleImGui() {
	ImGui_ImplDX11_NewFrame();

	bool show_test_window = true;
	bool show_another_window = true;
	ImVec4 clear_col = ImColor(114, 144, 154);
	// 1. Show a simple window
	// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
	{
		for (int z = 0; z < 3; z++)
		{
			for (int y = 0; y < 2; y++)
			{
				for (int x = 0; x < 2; x++)
				{
					//ImGui::Text(std::to_string(myNoise.GetNoise(x, y, z)).c_str());

					ImGui::Text(std::to_string(noiseMap[z*cornerDepth*cornerWidth + y * cornerWidth + x]).c_str());

				}
			}
			ImGui::Text("\n");
		}

		/*static float f = 0.0f;
		for (int i = 0; i < 12; i++) {
			ImGui::Text(std::to_string((i << 1) & 2).c_str());
			ImGui::Text(std::to_string(i & 2).c_str());
			ImGui::Text(std::to_string(i % 4).c_str());
			ImGui::Text("\n");
		}


		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);*/
		//ImGui::ColorEdit3("clear color", (float*)&clear_col);
		//if (ImGui::Button("Test Window")) show_test_window ^= 1;
		//if (ImGui::Button("Another Window")) show_another_window ^= 1;
		//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}

	//// 2. Show another simple window, this time using an explicit Begin/End pair
	//if (show_another_window)
	//{
	//	ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
	//	ImGui::Begin("Another Window", &show_another_window);
	//	ImGui::Text("Hello");
	//	ImGui::End();
	//}

	//// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	//if (show_test_window)
	//{
	//	ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);     // Normally user code doesn't need/want to call it because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
	//	ImGui::ShowTestWindow(&show_test_window);
	//}
}
