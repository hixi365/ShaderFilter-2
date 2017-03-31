#include "pch.h"
#include "SceneRenderer.h"

#include "WICTextureLoader.h"
#include "DDSTextureLoader.h"

using namespace DirectX;
using namespace Windows::Foundation;

//====================================================================================================
//	【コンストラクタ】
//	ファイルから頂点とピクセル シェーダーを読み込み、キューブのジオメトリをインスタンス化します。
//====================================================================================================
SceneRenderer::SceneRenderer(const std::shared_ptr<DeviceResources>& deviceResources, const std::shared_ptr<MainResources>& mainRsources) :
m_loadingComplete(false),
m_degreesPerSecond(45),
m_tracking(false),
m_deviceResources(deviceResources),
m_mainResources(mainRsources)
{

	m_loadingComplete = true;

	auto context = m_deviceResources->GetD3DDeviceContext();
	auto device = m_deviceResources->GetD3DDevice();

	HRESULT hr = S_OK;

	//------------------------------------------------------------------------------------
	// モデルの読み込み
	//------------------------------------------------------------------------------------
	m_MeshPlane = std::unique_ptr<CMesh11>(new CMesh11(m_deviceResources));
	m_MeshPlane->CreatePlane(1.0f);

	//------------------------------------------------------------------------------------
	// テクニック
	//------------------------------------------------------------------------------------
	m_TechMain = std::make_shared<CTechnique>(m_deviceResources);
	m_TechMain->Init(TECH_2D_MAIN);

	//------------------------------------------------------------------------------------
	// テクスチャ
	//------------------------------------------------------------------------------------

}

//====================================================================================================
//	【デストラクタ】
//====================================================================================================
SceneRenderer::~SceneRenderer()
{

}

//====================================================================================================
//====================================================================================================
void SceneRenderer::Update()
{


}

//====================================================================================================
//====================================================================================================
void SceneRenderer::Render()
{
	if (!m_loadingComplete)		{ return; }

	auto context = m_deviceResources->GetD3DDeviceContext();

	DrawTexture(m_TechMain, m_mainResources->GetOutSRV());

}

//====================================================================================================
//	スクリーン全面にテクスチャを描画
//====================================================================================================
void SceneRenderer::DrawTexture(const std::shared_ptr<CTechnique>& technique, ID3D11ShaderResourceView *pSRV)
{

	CBuffer CData;

	STATESET states = CTechnique::GetStates(RS_SOLID_CULLBACK, DS_DISABLEDEPTHTESTWRITE, BS_NONE);
	auto pSampler = CTechnique::GetSampler(SS_ANISOTROPIC_CLAMP);

	m_MeshPlane->Render(technique, states, &CData, &pSRV, 1, &pSampler, 1);

}