#pragma once

#include "pch.h"
#include "DeviceResources.h"

//---------------------------------------------------------
//	テクニック
//---------------------------------------------------------
typedef enum TECHNIQUE {
	
	TECH_2D_MAIN,					// テクスチャ コピー

	TECH_2D_BLUR,					// テクスチャ ぼかし
	TECH_2D_GAMMA,					// テクスチャ ガンマ
	TECH_2D_UNSHARP,				// テクスチャ 先鋭化

//	TECH_2D_LIFEGAME,				// ライフゲーム


} TECHNIQUE;


struct STATESET {
	STATESET() {
		RSState = nullptr;
		DSState = nullptr;
		BLState = nullptr;
	}
	STATESET(ID3D11RasterizerState1* rsState, ID3D11DepthStencilState* dsState, ID3D11BlendState* blState) {
		RSState = rsState;
		DSState = dsState;
		BLState = blState;
	}
	ID3D11RasterizerState1*		RSState;
	ID3D11DepthStencilState*	DSState;
	ID3D11BlendState*			BLState;
};

typedef enum FOR_FLAG
{
	FOR_VS = 0x1L,
	FOR_GS = 0x2L,
	FOR_PS = 0x4L,
	FOR_CS = 0x8L,
} FOR_FLAG;


// ラスタライザーステート
enum RSSTATE {
	RS_SOLID_CULLBACK,					// [0] SOLID, 背面カリング
	RS_SOLID_CULLNONE,					// [1] SOLID, カリングなし
};

// デプスステンシルステート
enum DSSTATE {
	DS_ENABLEDEPTHTESTWRITE,			// [0] 深度比較・深度書き込みあり
	DS_DISABLEDEPTHTESTWRITE,			// [1] 深度比較・深度書き込みなし
};

// サンプラーステート
enum SSSTATE {
	SS_POINT,							// [0] POINT
	SS_LINEAR_CLAMP,					// [1] LINEAR / CLAMP
	SS_ANISOTROPIC_CLAMP,				// [2] ANISOTROPIC
};


// ブレンドステート
enum BLSTATE {
	BS_NONE,							// [0] ブレンドなし
	BS_ALPHA,							// [1] ALPHA透過
	BS_ADD,								// [2] 加算(RGBA)
};

//--------------------------------------------------------------------------------------
//	構造体定義
//--------------------------------------------------------------------------------------

// コンスタントバッファ (float4*3 int4*3)
struct CBuffer
{
	DirectX::XMFLOAT4	float4A;		// 汎用(float型)
	DirectX::XMFLOAT4	float4B;		// 汎用(float型)
	DirectX::XMFLOAT4	float4C;		// 汎用(float型)
	DirectX::XMINT4		int4A;			// 汎用(int型)
	DirectX::XMINT4		int4B;			// 汎用(int型)
	DirectX::XMINT4		int4C;			// 汎用(int型)
};

//========================================================================================================
// 【クラスCTechnique】 
//========================================================================================================
class CTechnique
{
public:
	CTechnique(const std::shared_ptr<DeviceResources>& deviceResources);
	~CTechnique();

	void Reset();

	// アクセス関数
	ID3D11VertexShader*			GetVertexShader() const		 { return m_VShader.Get(); }
	ID3D11PixelShader*			GetPixelShader() const		 { return m_PShader.Get(); }
	ID3D11GeometryShader*		GetGeometryShader() const    { return m_GShader.Get(); }
	ID3D11ComputeShader*		GetComputeShader() const     { return m_CShader.Get(); }
	ID3D11InputLayout*			GetInputLayout() const		 { return m_InputLayout.Get(); }
	ID3D11RasterizerState1*		GetRasterizerState() const	 { return m_RasterizerState.Get(); }
	ID3D11DepthStencilState*	GetDepthStencilState() const { return m_DepthStencilState.Get(); }
	ID3D11Buffer*				GetConstBuffer() const		 { return m_CBuffer.Get(); }

	UINT						FlagCB() const		{ return m_flgCB; }
	UINT						FlagSRV() const		{ return m_flgSRV; }

	HRESULT	Init(TECHNIQUE tech);
	void	UpdataConstantBuffer(CBuffer &data);
	
	static STATESET GetStates(UINT rsID, UINT dsID, UINT bsID);
	static ID3D11SamplerState*	GetSampler(UINT ssID) { return m_SSList[ssID]; }

private:

	HRESULT LoadVertexShader(WCHAR *fileName, LPCSTR szEntryPoint, ID3D11VertexShader **ppShader,
		const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs=nullptr, UINT NumElements=0, ID3D11InputLayout **ppInputLayout=nullptr);

	HRESULT LoadGeometryShader(WCHAR *fileName, LPCSTR szEntryPoint, ID3D11GeometryShader **ppShader);
	HRESULT LoadPixelShader(WCHAR *fileName, LPCSTR szEntryPoint, ID3D11PixelShader **ppShader);
	HRESULT LoadComputeShader(WCHAR *fileName, LPCSTR szEntryPoint, ID3D11ComputeShader **ppShader);

	void	CreateDefaultStates();

	std::shared_ptr<DeviceResources>			m_deviceResources;

	Microsoft::WRL::ComPtr<ID3D11VertexShader>		m_VShader;
	Microsoft::WRL::ComPtr<ID3D11GeometryShader>	m_GShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>		m_PShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader>		m_CShader;

	Microsoft::WRL::ComPtr<ID3D11InputLayout>		m_InputLayout;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState1>	m_RasterizerState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	m_DepthStencilState;
	Microsoft::WRL::ComPtr<ID3D11BlendState>		m_BlendState;

	// 定数バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer>			m_CBuffer;		// シェーダに渡すリソース

	//---------------------------------------------------------------
	//	レンダリングステート
	//---------------------------------------------------------------
	static std::vector<ID3D11RasterizerState1*>		m_RSList;
	static std::vector<ID3D11DepthStencilState*>	m_DSList;
	static std::vector<ID3D11SamplerState*>			m_SSList;
	static std::vector<ID3D11BlendState1*>			m_BSList;
	static UINT										m_RefCount;


	UINT											m_flgCB = 0;			// 定数バッファの受け渡し先
	UINT											m_flgSRV = 0;			// SRV(サンプラー)の受け渡し先
};