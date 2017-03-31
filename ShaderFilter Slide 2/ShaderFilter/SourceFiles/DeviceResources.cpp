#include "pch.h"
#include "DeviceResources.h"

//====================================================================================================
//	コンストラクタ
//====================================================================================================
DeviceResources::DeviceResources() :
m_d3dFeatureLevel(D3D_FEATURE_LEVEL_11_0),
m_dpi(1.0f),
m_DisplayNo(-1)
{
	CreateDeviceIndependentResources();
	CreateDeviceResources();
}

//====================================================================================================
//	デストラクタ
//====================================================================================================
DeviceResources::~DeviceResources()
{
	for (UINT i = 0; i < m_SwapChain.size(); i++) {
		HRESULT hr;
		BOOL fullscreen;
		hr = m_SwapChain[i]->GetFullscreenState(&fullscreen, NULL);

		// フルスクリーンなら戻す
		if (fullscreen)
			hr = m_SwapChain[i]->SetFullscreenState(!fullscreen, NULL);
	}
}

//====================================================================================================
//	デバイスに依存しないリソースの作成 (再作成なし：インスタンス生成時のみ)
//====================================================================================================
HRESULT DeviceResources::CreateDeviceIndependentResources()
{
	HRESULT hr = S_OK;

	//-------------------------------------------------------------------
	// Direct2D リソースの初期化
	//-------------------------------------------------------------------
	D2D1_FACTORY_OPTIONS options;
	ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
	// Direct2D デバッグをSDKレイヤーを介して有効にする
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	// Direct2Dファクトリを初期化
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory2), &options, &m_d2dFactory);
	if (FAILED(hr)) { return hr; }

	// DirectWriteファクトリを初期化
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory2), &m_dwriteFactory);
	if (FAILED(hr)) { return hr; }

	// Windows Imaging Component (WIC) ファクトリを初期化
	hr = CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_wicFactory));
	if (FAILED(hr)) { return hr; }

	return hr;

}

//====================================================================================================
//	デバイスに依存するリソースの作成 (再作成なし：デバイスロストは考慮していない)
//====================================================================================================
HRESULT DeviceResources::CreateDeviceResources()
{
	HRESULT hr = S_OK;

	//---------------------------------------------------------------------------------
	//	D3D11デバイスとデバイスコンテキストを作成
	//---------------------------------------------------------------------------------
	UINT CreateFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	D3D_FEATURE_LEVEL FeatureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0, };

	//CreateFlags |= D3D10_CREATE_DEVICE_SINGLETHREADED;
	//CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

	// HAL
	hr = D3D11CreateDevice(
		nullptr,								// これでいいのか!!!!
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		CreateFlags,
		FeatureLevels,
		ARRAYSIZE(FeatureLevels),
		D3D11_SDK_VERSION, &device, &m_d3dFeatureLevel, &context
	);
	if (FAILED(hr)) {

		// REF
		hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_REFERENCE,
			NULL, CreateFlags, FeatureLevels, 1, D3D11_SDK_VERSION, &device, &m_d3dFeatureLevel, &context);
		if (FAILED(hr)) {
			MessageBox(NULL, L"Direct3D11デバイスの作成に失敗しました。", L"Error", MB_OK | MB_ICONEXCLAMATION);
			return hr;
		}
	}
	device.As(&m_d3dDevice);
	context.As(&m_d3dContext);

	//---------------------------------------------------------------------------------
	// Direct2Dデバイスとデバイスコンテキストを作成
	//---------------------------------------------------------------------------------

	// Direct2Dデバイス
	Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice;
	m_d3dDevice.As(&dxgiDevice);
	m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice);

	// Direct2Dデバイスコンテキストを作成
	m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext);

	IDXGIAdapter *adapter;
	DXGI_ADAPTER_DESC desc;
	dxgiDevice->GetAdapter(&adapter);
	adapter->GetDesc(&desc);
	wcscpy_s(m_adapterName, 128, desc.Description);


	return hr;
}

//====================================================================================================
//	ウィンドウサイズに依存するリソースの作成
//====================================================================================================
HRESULT DeviceResources::CreateWindowSizeDependentResources()
{
	HRESULT hr = S_OK;

	//---------------------------------------------------------------------------------
	// 前のウィンドウ サイズに固有のコンテキストをクリア
	//---------------------------------------------------------------------------------
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	m_d2dContext->SetTarget(nullptr);
	m_d3dContext->Flush();

	for (UINT i = 0; i < m_RenderTargetView.size(); i++)	m_RenderTargetView[i].Reset();
	for (UINT i = 0; i < m_DepthStencilView.size(); i++)	m_DepthStencilView[i].Reset();
	for (UINT i = 0; i < m_d2dTargetBitmap.size(); i++)		m_d2dTargetBitmap[i].Reset();

	// vectorをクリア
	m_RenderTargetView.clear();
	m_DepthStencilView.clear();
	m_Viewport.clear();
	m_RenderTargetSize.clear();
	m_d2dTargetBitmap.clear();

	//---------------------------------------------------------------------------------
	// スワップチェーンのサイズ変更または新規作成
	//---------------------------------------------------------------------------------
	Microsoft::WRL::ComPtr<IDXGIFactory2>	pDXGIFactory = nullptr;

	// DXGI Factoryインターフェイスの作成
	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pDXGIFactory);
	if (FAILED(hr))	return hr;


	for (UINT i = 0; i < m_hWnd.size(); i++) {

		RECT rc;
		HWND hWnd = m_hWnd[i];
		GetClientRect(hWnd, &rc);
		UINT W = rc.right - rc.left;
		UINT H = rc.bottom - rc.top;

		// 既にスワップチェーンがある場合はリサイズ
		if (m_SwapChain.size()>i) {
			HRESULT hr = m_SwapChain[i]->ResizeBuffers(
				2, // ダブル バッファーされたスワップ チェーン
				W, H, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
		}

		// それ以外の場合は新規作成
		else {
			Microsoft::WRL::ComPtr<IDXGISwapChain1> pSwapChain = nullptr;

			// ウィンドウモード
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
			swapChainDesc.Width			= W;
			swapChainDesc.Height		= H;
			swapChainDesc.Format		= DXGI_FORMAT_B8G8R8A8_UNORM;			// フォーマット
			swapChainDesc.Stereo		= false;
			swapChainDesc.SampleDesc.Count = 1; // マルチサンプリングなし
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.BufferUsage	= DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount	= 2; // ダブル バッファーを使用
			swapChainDesc.SwapEffect	= DXGI_SWAP_EFFECT_DISCARD;// DXGI_SWAP_EFFECT_DISCARD;// DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			swapChainDesc.Flags			= 0;// DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			swapChainDesc.Scaling		= DXGI_SCALING_STRETCH;
			swapChainDesc.AlphaMode		= DXGI_ALPHA_MODE_IGNORE;

			// フルスクリーン
			DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc = { 0 };
			swapChainFullScreenDesc.RefreshRate.Denominator = 1;
			swapChainFullScreenDesc.RefreshRate.Numerator = 60;
			swapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			swapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			swapChainFullScreenDesc.Windowed = true;// (flgFullscreenBoot == false);


			// ウィンドウからスワップチェーンを作成
			hr = pDXGIFactory->CreateSwapChainForHwnd(
					m_d3dDevice.Get(),
					hWnd,
					&swapChainDesc,
					&swapChainFullScreenDesc,
					nullptr,
					&pSwapChain	);
			if (FAILED(hr))	return hr;

			m_SwapChain.push_back(pSwapChain);
		}

	}

	//---------------------------------------------------------------------------------
	// スワップ チェーンに関連するリソースを作成
	//---------------------------------------------------------------------------------
	for (UINT i = 0; i < m_SwapChain.size(); i++) {
		Microsoft::WRL::ComPtr<ID3D11Texture2D>	backBuffer = nullptr;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRTV = nullptr;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> pDSV = nullptr;
		D3D11_VIEWPORT viewport;

		HWND hWnd;
		RECT rc;
		m_SwapChain[i]->GetHwnd(&hWnd);			// スワップチェーンに関連づいたウィンドウのハンドルを取得
		GetClientRect(hWnd, &rc);
		UINT W = rc.right - rc.left;
		UINT H = rc.bottom - rc.top;

		// バックバッファのレンダーターゲットビュー
		m_SwapChain[i]->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		hr = m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, pRTV.GetAddressOf());

		// 深度ステンシルビュー
		Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil = nullptr;
		CD3D11_TEXTURE2D_DESC depthStencilDesc(
			DXGI_FORMAT_D24_UNORM_S8_UINT, W, H,
			1, // この深度ステンシル ビューには、1 つのテクスチャしかありません。
			1, // 1 つの MIPMAP レベルを使用します。
			D3D11_BIND_DEPTH_STENCIL
			);
		hr = m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencil);

		CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
		hr = m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, pDSV.GetAddressOf());

		// ビューポート(ターゲット全体)
		viewport = CD3D11_VIEWPORT(0.0f, 0.0f, (FLOAT)W, (FLOAT)H);

		// vectorにpuch_back
		m_RenderTargetView.push_back(pRTV);
		m_DepthStencilView.push_back(pDSV);
		m_Viewport.push_back(viewport);

		SIZE size = { W, H };
		m_RenderTargetSize.push_back(size);

		//---------------------------------------------------------------------------------
		// スワップ チェーン バック バッファーに関連付けられた Direct2D ターゲット ビットマップを作成し、
		// それを現在のターゲットとして設定します。
		//---------------------------------------------------------------------------------
		Microsoft::WRL::ComPtr<ID2D1Bitmap1> pBitmap = nullptr;
		D2D1_BITMAP_PROPERTIES1 bitmapProperties =
			D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			m_dpi,
			m_dpi
			);

		Microsoft::WRL::ComPtr<IDXGISurface2> dxgiBackBuffer;
		hr = m_SwapChain[i]->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));

		hr = m_d2dContext->CreateBitmapFromDxgiSurface(
			dxgiBackBuffer.Get(),
			&bitmapProperties,
			&pBitmap
			);
		m_d2dTargetBitmap.push_back(pBitmap);

		D2D1_SIZE_U s = pBitmap->GetPixelSize();

	}


	return hr;
}

//====================================================================================================
//====================================================================================================
HRESULT DeviceResources::SetRenderTarget(int no)
{
	if (no < 0)	return E_FAIL;
	if (m_RenderTargetView.size() <= (UINT)no)	return E_FAIL;
	if (m_d2dTargetBitmap.size() <= (UINT)no)	return E_FAIL;

	m_DisplayNo = no;

	m_d2dContext->SetTarget(m_d2dTargetBitmap[no].Get());

	return S_OK;
}

//====================================================================================================
//	ウィンドウハンドルを登録
//====================================================================================================
void DeviceResources::RegisterWindow(HWND hWnd)
{
	m_hWnd.push_back(hWnd);

	// その都度リソースを再作成
	CreateWindowSizeDependentResources();
}

//====================================================================================================
//	ウィンドウ ⇔ フルスクリーン変換
//====================================================================================================
HRESULT DeviceResources::ToggleFullscreen()
{
	HRESULT hr = S_OK;

	for (UINT i = 0; i < m_SwapChain.size(); i++) {
		BOOL fullscreen;
		IDXGIOutput *pOutput = nullptr;

		hr = m_SwapChain[i]->GetFullscreenState(&fullscreen, &pOutput);
		hr = m_SwapChain[i]->SetFullscreenState(!fullscreen, NULL);
		if (FAILED(hr))	return hr;
	}

	return hr;
}

//====================================================================================================
//	ウィンドウ ⇔ フルスクリーン変換 (個別)
//====================================================================================================
HRESULT DeviceResources::ToggleFullscreen(UINT no)
{
	HRESULT hr = S_OK;

	if (no >= m_SwapChain.size())	return E_FAIL;

	BOOL fullscreen;
	IDXGIOutput *pOutput = nullptr;

	hr = m_SwapChain[no]->GetFullscreenState(&fullscreen, NULL);


	if (fullscreen == TRUE)
		hr = m_SwapChain[no]->SetFullscreenState(FALSE, NULL);
	else
		hr = m_SwapChain[no]->SetFullscreenState(TRUE, NULL);

	return hr;
}