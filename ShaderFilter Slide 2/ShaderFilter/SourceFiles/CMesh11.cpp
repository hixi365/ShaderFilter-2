#include "pch.h"

#include "CMesh11.h"
#include "WICTextureLoader.h"
#include "DDSTextureLoader.h"
#include "d3dcompiler.h"

using namespace DirectX;
using namespace Windows::Foundation;

using namespace Microsoft::WRL;


//========================================================================================================
//	【コンストラクタ】
//========================================================================================================
CMesh11::CMesh11(const std::shared_ptr<DeviceResources>& deviceResources) :
m_NumVertices(0),
m_NumIndices(0),
m_Complete(false),
m_ConstBuffer(nullptr),
m_deviceResources(deviceResources),
m_Stride(0)
{
	HRESULT hr = S_OK;

	auto device = m_deviceResources->GetD3DDevice();

	//--------------------------------------------------------------------------------
	// 定数バッファ作成
	//--------------------------------------------------------------------------------
	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(CBuffer), D3D11_BIND_CONSTANT_BUFFER);
	device->CreateBuffer(&constantBufferDesc, nullptr, m_ConstBuffer.GetAddressOf());

	//------------------------------------------------------------------------------------
	// インスタンシング用のバッファ
	//------------------------------------------------------------------------------------
	D3D11_BUFFER_DESC bufDesc;
	bufDesc.ByteWidth = sizeof(MeshInstData) * MeshInstDataMAX;
	bufDesc.Usage = D3D11_USAGE_DEFAULT;
	bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufDesc.CPUAccessFlags = 0;
	bufDesc.MiscFlags = 0;
	hr = device->CreateBuffer(&bufDesc, nullptr, m_InstBuffer.GetAddressOf());

	//--------------------------------------------------------------------------------
	// テクスチャのロード
	//--------------------------------------------------------------------------------
	CreateWICTextureFromFile(device, L"./texture/white.png", nullptr, m_WhiteSRV.GetAddressOf());		// 白テクスチャ(SRVがnullのとき使用)

}



//========================================================================================================
//	【デストラクタ】
//========================================================================================================
CMesh11::~CMesh11()
{
	m_ConstBuffer.Reset();
	m_VertexBuffer.Reset();
	m_IndexBuffer.Reset();
}

//========================================================================================================
//	インスタンシングデータのセット
//========================================================================================================
HRESULT CMesh11::SetMeshInstData(void *pData, UINT numMeshInstData)
{
	if (!m_InstBuffer.Get())	return E_FAIL;
	m_NumMeshInstData = numMeshInstData;
	if (m_NumMeshInstData == 0)	return S_OK;

	if (m_NumMeshInstData > MeshInstDataMAX)	m_NumMeshInstData = MeshInstDataMAX;

	auto context = m_deviceResources->GetD3DDeviceContext();

	context->UpdateSubresource(m_InstBuffer.Get(), 0, nullptr, pData, sizeof(MeshInstData)*m_NumMeshInstData, 1);

	return S_OK;
}

//========================================================================================================
//	メッシュの描画(テクスチャつきでメッシュを描画)
//========================================================================================================
void CMesh11::Render(const std::shared_ptr<CTechnique>& technique, STATESET &states, CBuffer* cbData, ID3D11ShaderResourceView* pSRVs[], UINT numSRVs, ID3D11SamplerState* pSamplers[], UINT numSamplers)
{
	if (!m_Complete)	return;

	auto context = m_deviceResources->GetD3DDeviceContext();

	bool UseIndex = (m_NumIndices>0) ? true : false;

	//--------------------------------------------------------------------------------
	// 頂点・インデックスバッファセット
	//--------------------------------------------------------------------------------
	UINT stride = m_Stride;
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(m_Topology);
	context->IASetInputLayout(technique->GetInputLayout());

	//--------------------------------------------------------------------------------
	// シェーダ
	//--------------------------------------------------------------------------------
	context->VSSetShader(technique->GetVertexShader(), nullptr, 0);
	context->GSSetShader(technique->GetGeometryShader(), nullptr, 0);
	context->PSSetShader(technique->GetPixelShader(), nullptr, 0);
	context->CSSetShader(technique->GetComputeShader(), nullptr, 0);

	//--------------------------------------------------------------------------------
	// ステート
	//--------------------------------------------------------------------------------
	FLOAT blendFactor[4] = { 0, 0, 0, 0 };
	if (states.RSState)	context->RSSetState(states.RSState);								// ラスタライザーステート
	if (states.DSState)	context->OMSetDepthStencilState(states.DSState, 0);					// デプスステンシルステート
	if (states.BLState)	context->OMSetBlendState(states.BLState, blendFactor, 0xFFFFFFFF);	// ブレンドステート

	//--------------------------------------------------------------------------------
	// シェーダーリソース
	//--------------------------------------------------------------------------------

	// 定数バッファ
	if (cbData) {
		technique->UpdataConstantBuffer(*cbData);
		ID3D11Buffer* pCBuffer = technique->GetConstBuffer();
		UINT flagCB = technique->FlagCB();
		if (flagCB & FOR_VS)	context->VSSetConstantBuffers(0, 1, &pCBuffer);
		if (flagCB & FOR_GS)	context->GSSetConstantBuffers(0, 1, &pCBuffer);
		if (flagCB & FOR_PS)	context->PSSetConstantBuffers(0, 1, &pCBuffer);
		if (flagCB & FOR_CS)	context->CSSetConstantBuffers(0, 1, &pCBuffer);
	}

	// テクスチャSRV
	if (pSRVs && numSamplers > 0) {
		UINT flagSRV = technique->FlagSRV();
		if (flagSRV & FOR_VS)	context->VSSetShaderResources(0, numSRVs, pSRVs);
		if (flagSRV & FOR_GS)	context->GSSetShaderResources(0, numSRVs, pSRVs);
		if (flagSRV & FOR_PS)	context->PSSetShaderResources(0, numSRVs, pSRVs);
		if (flagSRV & FOR_CS)	context->CSSetShaderResources(0, numSRVs, pSRVs);
	}

	// サンプラー
	if (pSamplers && numSamplers > 0) {
		UINT flagSAM = technique->FlagSRV();
		if (flagSAM & FOR_VS)	context->VSSetSamplers(0, numSamplers, pSamplers);
		if (flagSAM & FOR_GS)	context->GSSetSamplers(0, numSamplers, pSamplers);
		if (flagSAM & FOR_PS)	context->PSSetSamplers(0, numSamplers, pSamplers);
		if (flagSAM & FOR_CS)	context->CSSetSamplers(0, numSamplers, pSamplers);
	}

	//--------------------------------------------------------------------------------
	// オブジェクトを描画
	//--------------------------------------------------------------------------------
	if (UseIndex) 	context->DrawIndexed(m_NumIndices, 0, 0);
	else			context->Draw(m_NumVertices, 0);
}

//========================================================================================================
//	メッシュの描画(テクスチャつきでメッシュを描画)
//========================================================================================================
void CMesh11::RenderInstanced(const std::shared_ptr<CTechnique>& technique, STATESET &states, CBuffer* cbData, ID3D11ShaderResourceView* pSRVs[], UINT numSRVs, ID3D11SamplerState* pSamplers[], UINT numSamplers)
{
	if (!m_Complete)	return;

	auto context = m_deviceResources->GetD3DDeviceContext();

	bool UseIndex = (m_NumIndices>0) ? true : false;

	//--------------------------------------------------------------------------------
	// 頂点・インデックスバッファセット
	//--------------------------------------------------------------------------------
	
	// 頂点バッファをセット(モデルメッシュ+ワールド行列)
	ID3D11Buffer* pBuf[2] = { m_VertexBuffer.Get(), m_InstBuffer.Get() };
	UINT stride[2] = { m_Stride, sizeof(MeshInstData) };
	UINT offset[2] = { 0, 0 };
	context->IASetVertexBuffers(0, 2, pBuf, stride, offset);
	context->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(m_Topology);
	context->IASetInputLayout(technique->GetInputLayout());

	//--------------------------------------------------------------------------------
	// シェーダ
	//--------------------------------------------------------------------------------
	context->VSSetShader(technique->GetVertexShader(), nullptr, 0);
	context->GSSetShader(technique->GetGeometryShader(), nullptr, 0);
	context->PSSetShader(technique->GetPixelShader(), nullptr, 0);
	context->CSSetShader(technique->GetComputeShader(), nullptr, 0);

	//--------------------------------------------------------------------------------
	// ステート
	//--------------------------------------------------------------------------------
	FLOAT blendFactor[4] = { 0, 0, 0, 0 };
	if (states.RSState)	context->RSSetState(states.RSState);								// ラスタライザーステート
	if (states.DSState)	context->OMSetDepthStencilState(states.DSState, 0);					// デプスステンシルステート
	if (states.BLState)	context->OMSetBlendState(states.BLState, blendFactor, 0xFFFFFFFF);	// ブレンドステート

	//--------------------------------------------------------------------------------
	// シェーダーリソース
	//--------------------------------------------------------------------------------

	// 定数バッファ
	if (cbData) {
		technique->UpdataConstantBuffer(*cbData);
		ID3D11Buffer* pCBuffer = technique->GetConstBuffer();
		UINT flagCB = technique->FlagCB();
		if (flagCB & FOR_VS)	context->VSSetConstantBuffers(0, 1, &pCBuffer);
		if (flagCB & FOR_GS)	context->GSSetConstantBuffers(0, 1, &pCBuffer);
		if (flagCB & FOR_PS)	context->PSSetConstantBuffers(0, 1, &pCBuffer);
		if (flagCB & FOR_CS)	context->CSSetConstantBuffers(0, 1, &pCBuffer);
	}

	// テクスチャSRV
	if (pSRVs && numSamplers > 0) {
		UINT flagSRV = technique->FlagSRV();
		if (flagSRV & FOR_VS)	context->VSSetShaderResources(0, numSRVs, pSRVs);
		if (flagSRV & FOR_GS)	context->GSSetShaderResources(0, numSRVs, pSRVs);
		if (flagSRV & FOR_PS)	context->PSSetShaderResources(0, numSRVs, pSRVs);
		if (flagSRV & FOR_CS)	context->CSSetShaderResources(0, numSRVs, pSRVs);
	}


	// サンプラー
	if (pSamplers && numSamplers > 0) {
		UINT flagSAM = technique->FlagSRV();
		if (flagSAM & FOR_VS)	context->VSSetSamplers(0, numSamplers, pSamplers);
		if (flagSAM & FOR_GS)	context->GSSetSamplers(0, numSamplers, pSamplers);
		if (flagSAM & FOR_PS)	context->PSSetSamplers(0, numSamplers, pSamplers);
		if (flagSAM & FOR_CS)	context->CSSetSamplers(0, numSamplers, pSamplers);
	}

	//--------------------------------------------------------------------------------
	// オブジェクトを描画
	//--------------------------------------------------------------------------------
	if (UseIndex) 		context->DrawIndexedInstanced(m_NumIndices, m_NumMeshInstData, 0, 0, 0);

}

//========================================================================================================
//	メッシュの描画
//========================================================================================================
void CMesh11::RenderInstancing(CBuffer& mat, const std::shared_ptr<CTechnique>& technique, ComPtr<ID3D11SamplerState> SampleState, ComPtr<ID3D11BlendState> BlendState)
{
	if (!m_Complete)	return;

	auto context = m_deviceResources->GetD3DDeviceContext();

	//--------------------------------------------------------------------------------
	// 頂点・インデックスバッファセット
	//--------------------------------------------------------------------------------

	// 頂点バッファをセット(モデルメッシュ+ワールド行列)
	ID3D11Buffer* pBuf[2] = { m_VertexBuffer.Get(), m_InstBuffer.Get() };
	UINT stride[2] = { m_Stride, sizeof(MeshInstData) };
	UINT offset[2] = { 0, 0 };
	context->IASetVertexBuffers(0, 2, pBuf, stride, offset);
	context->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(m_Topology);
	context->IASetInputLayout(technique->GetInputLayout());

	//--------------------------------------------------------------------------------
	// シェーダ
	//--------------------------------------------------------------------------------
	context->VSSetShader(technique->GetVertexShader(), nullptr, 0);
	context->GSSetShader(technique->GetGeometryShader(), nullptr, 0);
	context->PSSetShader(technique->GetPixelShader(), nullptr, 0);
	context->CSSetShader(technique->GetComputeShader(), nullptr, 0);

	//--------------------------------------------------------------------------------
	// 定数バッファーを更新
	//--------------------------------------------------------------------------------
	
	context->UpdateSubresource(m_ConstBuffer.Get(), 0, nullptr, &mat, 0, 0);
	context->VSSetConstantBuffers(0, 1, m_ConstBuffer.GetAddressOf());		// 定数バッファ

	//--------------------------------------------------------------------------------
	// ステート
	//--------------------------------------------------------------------------------
	context->RSSetState(technique->GetRasterizerState());						// ラスタライザーステート
	context->OMSetDepthStencilState(technique->GetDepthStencilState(), 0);		// 深度ステンシルステート
	context->PSSetSamplers(0, 1, SampleState.GetAddressOf());				// サンプラーステート
	FLOAT blendFactor[4] = { 0, 0, 0, 0 };
	context->OMSetBlendState(BlendState.Get(), blendFactor, 0xFFFFFFFF);	// ブレンドステート

	//--------------------------------------------------------------------------------
	// シェーダリソース
	//--------------------------------------------------------------------------------
	context->PSSetShaderResources(0, 1, m_SRV.GetAddressOf());					// テクスチャ(SRV)

	//--------------------------------------------------------------------------------
	// オブジェクトを描画
	//--------------------------------------------------------------------------------
	context->DrawIndexedInstanced(m_NumIndices, m_NumMeshInstData, 0, 0, 0);

}

//========================================================================================================
//	【平面】
//========================================================================================================
void CMesh11::CreatePlane(float size)
{
	HRESULT hr = S_OK;

	float s = size;

	// メッシュの頂点データ { 頂点座標, 頂点色 }
	m_NumVertices = 4;
	MeshVertex *v = new MeshVertex[m_NumVertices];
	v[0].pos = XMFLOAT3(-s, s, 0);	v[0].normal = XMFLOAT3(0, 0, 1);	v[0].color = XMFLOAT4(1, 1, 1, 1);	v[0].tex = XMFLOAT2(0, 0);
	v[1].pos = XMFLOAT3(s, s, 0);	v[1].normal = XMFLOAT3(0, 0, 1);	v[1].color = XMFLOAT4(1, 1, 1, 1);	v[1].tex = XMFLOAT2(1, 0);
	v[2].pos = XMFLOAT3(-s, -s, 0);	v[2].normal = XMFLOAT3(0, 0, 1);	v[2].color = XMFLOAT4(1, 1, 1, 1);	v[2].tex = XMFLOAT2(0, 1);
	v[3].pos = XMFLOAT3(s, -s, 0);	v[3].normal = XMFLOAT3(0, 0, 1);	v[3].color = XMFLOAT4(1, 1, 1, 1);	v[3].tex = XMFLOAT2(1, 1);


	D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
	vertexBufferData.pSysMem = v;
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;

	CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(MeshVertex)*m_NumVertices, D3D11_BIND_VERTEX_BUFFER);
	hr = m_deviceResources->GetD3DDevice()->CreateBuffer(
		&vertexBufferDesc,
		&vertexBufferData,
		&m_VertexBuffer
		);

	delete[] v;

	// メッシュのインデックスデータ(CCW)
	static const DWORD Indices[] =
	{
		0, 2, 1, 1, 2, 3,
	};

	// インデックスバッファの作成
	m_NumIndices = ARRAYSIZE(Indices);

	D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
	indexBufferData.pSysMem = Indices;
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC indexBufferDesc(sizeof(Indices), D3D11_BIND_INDEX_BUFFER);
	hr = m_deviceResources->GetD3DDevice()->CreateBuffer(
		&indexBufferDesc, &indexBufferData, &m_IndexBuffer);

	m_Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_Stride = sizeof(MeshVertex);

	m_Complete = true;
}

//========================================================================================================
//	ID用
//========================================================================================================
void CMesh11::CreateIDPack(UINT numIDs)
{

	HRESULT hr = S_OK;

	IDPack *pData = new IDPack[numIDs];

	for (UINT i = 0; i < numIDs; i++) {
		pData[i].id = i;
	}

	D3D11_SUBRESOURCE_DATA initialData;

	initialData.pSysMem = pData;
	initialData.SysMemPitch = 0;
	initialData.SysMemSlicePitch = 0;

	m_NumVertices = numIDs;

	//--------------------------------------------------------------------------------
	//	ID用バッファ
	//--------------------------------------------------------------------------------
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.ByteWidth = sizeof(IDPack)* numIDs;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	hr = m_deviceResources->GetD3DDevice()->CreateBuffer(&bufferDesc, &initialData, m_VertexBuffer.GetAddressOf());

	//------------------------------------------------------------------------------------
	//	PRIMITIVE_TOPOLOGY
	//------------------------------------------------------------------------------------
	m_Topology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	m_Stride = sizeof(IDPack);

	m_Complete = true;

}