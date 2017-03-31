#pragma once

#include "DeviceResources.h"
#include "MainResources.h"

#include "CMesh11.h"

//========================================================================================================
// 【クラスSceneRenderer】 シーン描画クラス
//========================================================================================================
class SceneRenderer
{
public:
	SceneRenderer(const std::shared_ptr<DeviceResources>& deviceResources, const std::shared_ptr<MainResources>& mainRsources);		// コンストラクタ
	~SceneRenderer();															// デストラクタ
		
	void	Update();
	void	Render();

private:

	void	DrawTexture(const std::shared_ptr<CTechnique>& technique, ID3D11ShaderResourceView *pSRV);

private:
	// 共有リソース
	std::shared_ptr<DeviceResources>				m_deviceResources;
	std::shared_ptr<MainResources>					m_mainResources;

	// テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_TestSRV;

	// メッシュ
	std::unique_ptr<CMesh11>							m_MeshPlane;


	// テクニック
	std::shared_ptr<CTechnique>							m_TechMain;


	//---------------------------------------------------------------
	// レンダリング ループで使用する変数。
	//---------------------------------------------------------------
	bool	m_loadingComplete;
	float	m_degreesPerSecond;
	bool	m_tracking;
	bool	m_RTTexture = false;

};
