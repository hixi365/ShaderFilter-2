
//--------------------------------------------------------------------------------
//	ヘッダ・ファイル
//--------------------------------------------------------------------------------
#include <stdlib.h>
#include <math.h>

#include "pch.h"
#include "MainResources.h"

#include <WICTextureLoader.h>
#include <ScreenGrab.h>

//--------------------------------------------------------------------------------
//	大域変数
//--------------------------------------------------------------------------------

extern UINT g_WindowWidth;
extern UINT g_WindowHeight;

//========================================================================================================
//	コンストラクタ
//========================================================================================================
MainResources::MainResources(const std::shared_ptr<DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources)
{

	// シェーダの作成
	CreateTechResources();

	// メッシュの作成
	CreateMeshResources();

	// ライフゲーム用テクスチャの作成
	CreateLifeGameResources();

}

//========================================================================================================
//	デストラクタ
//========================================================================================================
MainResources::~MainResources()
{

}

//========================================================================================================
//	リソースの更新
//========================================================================================================
void MainResources::Update()
{

	if(m_flgLifeGame)
		UpdateLifeGame();
	
}

//========================================================================================================
//	Shader関係のリソース
//========================================================================================================
void MainResources::CreateTechResources()
{

	m_TechLifeGame = std::make_shared<CTechnique>(m_deviceResources);
	m_TechLifeGame->Init(TECH_2D_LIFEGAME);

}

//========================================================================================================
//	Mesh関係のリソース
//========================================================================================================
void MainResources::CreateMeshResources()
{

	m_MeshPlane = std::unique_ptr<CMesh11>(new CMesh11(m_deviceResources));
	m_MeshPlane->CreatePlane(1.0f);

}

//====================================================================================================
//	テクスチャを保存
//====================================================================================================
void MainResources::SaveTexture()
{

	auto context = m_deviceResources->GetD3DDeviceContext();
	HRESULT hr = S_OK;

	{
		WCHAR filename[256], tstamp[256];

		// 現在時刻を取得
		SYSTEMTIME	t;
		GetLocalTime(&t);

		// 保存フォルダ作成
		CreateDirectory(L"./Output", NULL);

		// タイムスタンプ
		swprintf_s(tstamp, 256, L"Screen-%04d%02d%02d%02d%02d%02d",
			t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

		Microsoft::WRL::ComPtr<ID3D11Texture2D> pTex = nullptr;

		swprintf_s(filename, 256, L"./Output/%s-out.png", tstamp);
		pTex = m_OutTex.Get();

		hr = DirectX::SaveWICTextureToFile(context, (ID3D11Resource*)pTex.Get(), GUID_ContainerFormatPng, filename);

	}

}

//========================================================================================================
//	ライフゲーム関係のリソース
//========================================================================================================
void MainResources::CreateLifeGameResources()
{
	HRESULT hr = S_OK;

	auto device = m_deviceResources->GetD3DDevice();
	auto context = m_deviceResources->GetD3DDeviceContext();

	//------------------------------------------------------------------------------------
	//　2Dテクスチャの作成
	//------------------------------------------------------------------------------------
	{

		D3D11_TEXTURE2D_DESC descTex;
		descTex.Width = TEXTURE_WIDTH;
		descTex.Height = TEXTURE_HEIGHT;
		descTex.MipLevels = 1;
		descTex.ArraySize = 1;
		descTex.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		descTex.SampleDesc.Count = 1;
		descTex.SampleDesc.Quality = 0;
		descTex.Usage = D3D11_USAGE_DEFAULT;
		descTex.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		descTex.CPUAccessFlags = 0;
		descTex.MiscFlags = 0;

		// RTV
		D3D11_RENDER_TARGET_VIEW_DESC descRTV;
		descRTV.Format = descTex.Format;
		descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		descRTV.Texture2D.MipSlice = 0;

		// SRV
		D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
		descSRV.Format = descTex.Format;
		descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		descSRV.Texture2D.MipLevels = 1;
		descSRV.Texture2D.MostDetailedMip = 0;

		// Out
		hr = device->CreateTexture2D(&descTex, nullptr, m_OutTex.GetAddressOf());
		hr = device->CreateRenderTargetView(m_OutTex.Get(), &descRTV, m_OutRTV.GetAddressOf());
		hr = device->CreateShaderResourceView(m_OutTex.Get(), &descSRV, m_OutSRV.GetAddressOf());

		// LifeGameOld
		hr = device->CreateTexture2D(&descTex, nullptr, m_LifeGameOldTex.GetAddressOf());
		hr = device->CreateRenderTargetView(m_LifeGameOldTex.Get(), &descRTV, m_LifeGameOldRTV.GetAddressOf());
		hr = device->CreateShaderResourceView(m_LifeGameOldTex.Get(), &descSRV, m_LifeGameOldSRV.GetAddressOf());

		// LifeGameNew
		hr = device->CreateTexture2D(&descTex, nullptr, m_LifeGameNewTex.GetAddressOf());
		hr = device->CreateRenderTargetView(m_LifeGameNewTex.Get(), &descRTV, m_LifeGameNewRTV.GetAddressOf());
		hr = device->CreateShaderResourceView(m_LifeGameNewTex.Get(), &descSRV, m_LifeGameNewSRV.GetAddressOf());

	}

	InitLifeGame();

}

//========================================================================================================
//	ライフゲームの更新
//========================================================================================================
void MainResources::UpdateLifeGame()
{

	auto context = m_deviceResources->GetD3DDeviceContext();

	D3D11_VIEWPORT vp = { 0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, 1 };
	D3D11_VIEWPORT pVPs[] = { vp };
	context->RSSetViewports(1, pVPs);

	CBuffer CData;
	CData.float4A.x = m_floatAX;
	float color[4] = { 0, 0, 0, 1 };

	ID3D11RenderTargetView   *pRTVs[] = { m_LifeGameNewRTV.Get() };
	ID3D11ShaderResourceView *pSRVs[] = { m_LifeGameOldSRV.Get() };

	context->OMSetRenderTargets(1, pRTVs, nullptr);
	context->ClearRenderTargetView(pRTVs[0], color);

	STATESET states = CTechnique::GetStates(RS_SOLID_CULLBACK, DS_DISABLEDEPTHTESTWRITE, BS_ALPHA);
	auto pSampler = CTechnique::GetSampler(SS_ANISOTROPIC_CLAMP);

	m_MeshPlane->Render(m_TechLifeGame, states, &CData, pSRVs, 1, &pSampler, 1);

	m_LifeGameNewTex.Swap(m_LifeGameOldTex);
	m_LifeGameNewRTV.Swap(m_LifeGameOldRTV);
	m_LifeGameNewSRV.Swap(m_LifeGameOldSRV);

	context->CopyResource(m_OutTex.Get(), m_LifeGameOldTex.Get());

}

//========================================================================================================
//	ライフゲームの初期化
//========================================================================================================
void MainResources::InitLifeGame(int pattern)
{

	if(pattern != -1)
		m_patternLifeGame = pattern;

	static XMFLOAT4 buf[TEXTURE_WIDTH*TEXTURE_HEIGHT];
	ZeroMemory(buf, sizeof(buf));

	auto context = m_deviceResources->GetD3DDeviceContext();

	switch (m_patternLifeGame)
	{
	case 0:

		// ランダム
		for (int i = 0; i < TEXTURE_WIDTH*TEXTURE_HEIGHT; i++) {
			float c = rand() % 3 == 0;
			buf[i] = XMFLOAT4(c, c, c, 1);
		}

		break;

	case 1:

		// グライダー銃
		int gun[] = {
			TEXTURE_WIDTH * 0 + 24, TEXTURE_WIDTH * 1 + 22, TEXTURE_WIDTH * 1 + 24, TEXTURE_WIDTH * 2 + 12, TEXTURE_WIDTH * 2 + 13, TEXTURE_WIDTH * 2 + 20, 
			TEXTURE_WIDTH * 2 + 21, TEXTURE_WIDTH * 2 + 34, TEXTURE_WIDTH * 2 + 35, TEXTURE_WIDTH * 3 + 11, TEXTURE_WIDTH * 3 + 15, TEXTURE_WIDTH * 3 + 20, 
			TEXTURE_WIDTH * 3 + 21, TEXTURE_WIDTH * 3 + 34, TEXTURE_WIDTH * 3 + 35, TEXTURE_WIDTH * 4 +  0, TEXTURE_WIDTH * 4 +  1, TEXTURE_WIDTH * 4 + 10,
			TEXTURE_WIDTH * 4 + 16, TEXTURE_WIDTH * 4 + 20, TEXTURE_WIDTH * 4 + 21, TEXTURE_WIDTH * 5 +  0, TEXTURE_WIDTH * 5 +  1, TEXTURE_WIDTH * 5 + 10,
			TEXTURE_WIDTH * 5 + 14, TEXTURE_WIDTH * 5 + 16, TEXTURE_WIDTH * 5 + 17, TEXTURE_WIDTH * 5 + 22, TEXTURE_WIDTH * 5 + 24, TEXTURE_WIDTH * 6 + 10, 
			TEXTURE_WIDTH * 6 + 16, TEXTURE_WIDTH * 6 + 24, TEXTURE_WIDTH * 7 + 11, TEXTURE_WIDTH * 7 + 15, TEXTURE_WIDTH * 8 + 12, TEXTURE_WIDTH * 8 + 13,
		};

		// 配置座標
		XMUINT2 pos[] = {
			XMUINT2( 10,  10),
			XMUINT2( 50,  10), XMUINT2( 90,  10), XMUINT2(130,  10), XMUINT2(170,  10), XMUINT2(210,  10), XMUINT2(250,  10), XMUINT2(290,  10), XMUINT2(330,  10), XMUINT2(370,  10),
			XMUINT2( 10,  50), XMUINT2( 10,  90), XMUINT2( 10, 130), XMUINT2( 10, 170), XMUINT2( 10, 210), XMUINT2( 10, 250), XMUINT2( 10, 290), XMUINT2( 10, 330), XMUINT2( 10, 370),
		};

		for(int j = 0; j < sizeof(pos) / sizeof(pos[0]); j++){
			for (int i = 0; i < sizeof(gun) / sizeof(gun[0]); i++) {
				int x = pos[j].x;
				int y = pos[j].y;
				buf[x + TEXTURE_WIDTH * y + gun[i]] = XMFLOAT4(1, 1, 1, 1);
			}
		}

		for (int j = 0; j < sizeof(pos) / sizeof(pos[0]); j++) {
			for (int i = 0; i < sizeof(gun) / sizeof(gun[0]); i++) {
				int x = 1025 - pos[j].x;
				int y = 1000 - pos[j].y;
				buf[x + TEXTURE_WIDTH * y - gun[i]] = XMFLOAT4(1, 1, 1, 1);
			}
		}

		break;

	}

//	context->UpdateSubresource(m_LifeGameOldTex.Get(), 0, nullptr, buf, sizeof(XMUINT4)*TEXTURE_WIDTH, sizeof(XMUINT4)*TEXTURE_WIDTH*TEXTURE_HEIGHT);

}