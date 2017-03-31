#pragma once

#include "pch.h"
#include "DeviceResources.h"
#include "CTechnique.h"

#define MeshInstDataMAX 100000

using namespace Microsoft::WRL;


//========================================================================================================
//	構造体
//========================================================================================================
	
//-------------------------------------------------------
// 頂点バッファ用構造体
//-------------------------------------------------------
struct MeshVertex
{
	DirectX::XMFLOAT3 pos;			// 位置
	DirectX::XMFLOAT3 normal;		// 法線ベクトル
	DirectX::XMFLOAT4 color;		// 色
	DirectX::XMFLOAT2 tex;			// テクスチャ座標
};

struct MeshInstData {
	DirectX::XMMATRIX mat;			// モデル行列
};

struct IDPack {
	UINT			id;				// id
};

//========================================================================================================
//	【CMesh11クラス】
//========================================================================================================
class CMesh11
{
public:
	CMesh11(const std::shared_ptr<DeviceResources>& deviceResources);
	~CMesh11();

	void Render(const std::shared_ptr<CTechnique>& technique, STATESET &states, CBuffer* cbData, ID3D11ShaderResourceView* pSRV[] = { nullptr }, UINT nSRVs = 0, ID3D11SamplerState* pSamplers[] = { nullptr }, UINT nSamplers = 0);
	void RenderInstanced(const std::shared_ptr<CTechnique>& technique, STATESET &states, CBuffer* cbData, ID3D11ShaderResourceView* pSRV[] = { nullptr }, UINT nSRVs = 0, ID3D11SamplerState* pSamplers[] = { nullptr }, UINT nSamplers = 0);

	void RenderInstancing(CBuffer&  mat, const std::shared_ptr<CTechnique>& technique, ComPtr<ID3D11SamplerState> SampleState, ComPtr<ID3D11BlendState> BlendState);

	ID3D11Buffer*				GetVertexBuffer() const { return m_VertexBuffer.Get(); }
	ID3D11UnorderedAccessView*	GetVertexUAV() const { return m_VertexUAV.Get(); }
	UINT						GetNumVertices() const { return m_NumVertices; }

	void	CreatePlane(float size);

	void	CreateIDPack(UINT numIDs);

	HRESULT SetMeshInstData(void* pData, UINT numMeshInstData);

private:

	std::shared_ptr<DeviceResources>					m_deviceResources;		// デバイス・リソースへのポインタ(share)

	Microsoft::WRL::ComPtr<ID3D11Buffer>				m_ConstBuffer;			// 定数バッファ

	// メッシュモデルに必要なリソース
	Microsoft::WRL::ComPtr<ID3D11Buffer>				m_VertexBuffer;			// 頂点バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer>				m_IndexBuffer;			// インデックス
	Microsoft::WRL::ComPtr<ID3D11Buffer>				m_InstBuffer;			// インスタンシング用バッファ
	UINT												m_NumMeshInstData;		// インスタンシングデータ数

	Microsoft::WRL::ComPtr<ID3D11Buffer>				m_VertexBufCpy;			// 頂点バッファのコピー(STAGING) → Particleのみ
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	m_VertexUAV;			// 頂点バッファアクセスUAV

	DWORD												m_NumVertices;			// 頂点数
	DWORD												m_NumIndices;			// インデックス数
	UINT												m_Stride = 0;			// 1頂点当たりのストライド(バイト数)

	D3D11_PRIMITIVE_TOPOLOGY							m_Topology;				// PRIMITIVE_TOPOLOGY

	// テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_WhiteSRV;


	// 管理用
	bool			m_Complete;

};