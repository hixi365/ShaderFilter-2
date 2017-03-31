#pragma once

#include "Main.h"

class DeviceResources
{
public:
	DeviceResources();			// コンストラクタ
	~DeviceResources();			// デストラクタ


	void	RegisterWindow(HWND hWnd);
	HRESULT	CreateWindowSizeDependentResources();
	HRESULT	ToggleFullscreen();
	HRESULT	ToggleFullscreen(UINT no);


	// アクセス関数
	ID3D11Device2*			GetD3DDevice() const					{ return m_d3dDevice.Get(); }
	ID3D11DeviceContext2*	GetD3DDeviceContext() const				{ return m_d3dContext.Get(); }
	D3D_FEATURE_LEVEL		GetDeviceFeatureLevel() const			{ return m_d3dFeatureLevel; }
	WCHAR*					GetAdapterName()						{ return m_adapterName; }

	IDXGISwapChain1*		GetSwapChain(UINT i) const				{ return m_SwapChain[i].Get(); }
	ID3D11RenderTargetView*	GetRenderTargetView(UINT i) const		{ return m_RenderTargetView[i].Get(); }
	ID3D11DepthStencilView* GetDepthStencilView(UINT i) const		{ return m_DepthStencilView[i].Get(); }
	D3D11_VIEWPORT			GetViewport(UINT i) const				{ return m_Viewport[i]; }
	SIZE					GetRenderTargetSize(UINT i) const		{ return m_RenderTargetSize[i]; }
	SIZE					GetCurrentRenderTargetSize() const		{ return m_RenderTargetSize[m_DisplayNo]; }

	// D2D アクセス関数
	ID2D1Factory2*			GetD2DFactory() const					{ return m_d2dFactory.Get(); }
	ID2D1Device1*			GetD2DDevice() const					{ return m_d2dDevice.Get(); }
	ID2D1DeviceContext1*	GetD2DDeviceContext() const				{ return m_d2dContext.Get(); }
	IDWriteFactory2*		GetDWriteFactory() const				{ return m_dwriteFactory.Get(); }
	IWICImagingFactory2*	GetWicImagingFactory() const			{ return m_wicFactory.Get(); }

	ID2D1Bitmap1*			GetD2DTargetBitmap(UINT i) const		{ return m_d2dTargetBitmap[i].Get(); }
	int						GetDisplayNo() const					{ return m_DisplayNo; }

	HRESULT					SetRenderTarget(int no);

private:

	HRESULT	CreateDeviceIndependentResources();				// デバイスに依存しないリソースの作成
	HRESULT	CreateDeviceResources();						// デバイス・デバイスコンテキストの作成


	//---------------------------------------------------------------
	//	DirectXリソース
	//---------------------------------------------------------------
	// D3D リソース
	Microsoft::WRL::ComPtr<ID3D11Device2>							m_d3dDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext2>					m_d3dContext;
	D3D_FEATURE_LEVEL												m_d3dFeatureLevel;
	WCHAR															m_adapterName[128];

	// D2D リソース
	Microsoft::WRL::ComPtr<ID2D1Factory2>							m_d2dFactory;
	Microsoft::WRL::ComPtr<ID2D1Device1>							m_d2dDevice;
	Microsoft::WRL::ComPtr<ID2D1DeviceContext1>						m_d2dContext;
	Microsoft::WRL::ComPtr<IDWriteFactory2>							m_dwriteFactory;
	Microsoft::WRL::ComPtr<IWICImagingFactory2>						m_wicFactory;

	// ウィンドウサイズに依存するリソース(複数作成)
	std::vector<HWND>												m_hWnd;
	std::vector<Microsoft::WRL::ComPtr<IDXGISwapChain1>>			m_SwapChain;
	std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>>		m_RenderTargetView;
	std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>>		m_DepthStencilView;
	std::vector<D3D11_VIEWPORT>										m_Viewport;
	std::vector<Microsoft::WRL::ComPtr<ID2D1Bitmap1>>				m_d2dTargetBitmap;
	std::vector<SIZE>												m_RenderTargetSize;

	float		m_dpi;
	int			m_DisplayNo = 0;

	UINT		m_MultiSampleCount = 1;		// マルチサンプルCOUNT
	UINT		m_MultiSampleQuality = 0;	// マルチサンプルQUALITY

};