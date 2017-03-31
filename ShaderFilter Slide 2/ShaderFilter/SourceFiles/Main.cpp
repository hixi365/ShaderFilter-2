
//-------------------------------------------------------------------------------------
//	ヘッダ・ファイル
//-------------------------------------------------------------------------------------
#include "pch.h"
#include "DeviceResources.h"
#include "MainResources.h"
#include "SceneRenderer.h"

//--------------------------------------------------------------------------------
//	システム・フラグ
//--------------------------------------------------------------------------------
bool	flgFirstRendering = true;		// 初回レンダリング
int		counterCaptureWindow = 0;		// キャプチャ対象ウィンドウ

//-------------------------------------------------------------------------------------
//	アダプタ関連
//-------------------------------------------------------------------------------------
std::vector <Microsoft::WRL::ComPtr<IDXGIOutput1>>	g_pOutput;

//-------------------------------------------------------------------------------------
//	ウィンドウ関連
//-------------------------------------------------------------------------------------
std::vector<HWND>						g_WinHandle;
std::vector<IDXGIOutputDuplication*>	g_pDeskDuplications;

//-------------------------------------------------------------------------------------
//	グローバル変数
//-------------------------------------------------------------------------------------
UINT		g_WindowWidth;
UINT		g_WindowHeight;

//---------------------------------------------------------------
//	マウス・タッチイベント関連
//---------------------------------------------------------------
HWND		g_hWndTouch = NULL;						// タッチ・イベントを起こすWindowのハンドル
HWND		g_hWndMouse = NULL;						// マウス・イベントを起こすWindowのハンドル

TOUCHINPUT	g_TouchInfo[100];						// タッチ情報
UINT		g_NumTouch = 0;

POINT		g_CursorPos;							// マウス(タッチ)カーソル座標(クライアント領域)
POINT		g_CursorPosOld;

bool		g_flgDown = false;

void		TouchEventProc(HTOUCHINPUT hTouchInput, UINT numInputs);
void		MouseEventProc(UINT uMsg, WPARAM wParam);

void		UIEvent(HWND hWnd, POINT p, bool flgDown, bool flgUp, bool flgMove);

//-------------------------------------------------------------------------------------
//	外部リソース
//-------------------------------------------------------------------------------------
std::shared_ptr<DeviceResources>	g_deviceResources;			// デバイスリソース
std::shared_ptr<MainResources>		g_mainResources;			// アプリケーションリソース

std::unique_ptr<SceneRenderer>		g_sceneRenderer;			// シーンレンダラー

//-------------------------------------------------------------------------------------
//	関数プロトタイプ
//-------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void	Cleanup();
HRESULT CheckAdapter();
//void	GetDuplications();
HRESULT SetupWindow(HINSTANCE hInstance);
HRESULT AppLoop();

void	Render();
void	Update();

void	SetClientSize(HWND hWnd, int width, int height);
void	SetupRenderTarget(int no);

void	GetScreenSize();

//====================================================================================================
// wWinMain	エントリーポイント
//====================================================================================================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{

	//------------------------------------------------------------------------------------
	//	アダプタ(ビデオカード)のチェック
	//------------------------------------------------------------------------------------
	if (FAILED(CheckAdapter())) {
		MessageBox(NULL, L"アプリケーションを強制終了します。", L"Error", MB_OK);
		return 1;
	}

	//------------------------------------------------------------------------------------
	//	ウィンドウのセットアップ　⇒ (Outputの数だけウィンドウを作成)
	//------------------------------------------------------------------------------------
	if (FAILED(SetupWindow(hInstance))) {
		MessageBox(NULL, L"ウィンドウのセットアップに失敗しました。アプリケーションを強制終了します。", L"Error", MB_OK | MB_ICONEXCLAMATION);
		Cleanup();
		return 1;
	}

	//------------------------------------------------------------------------------------
	//	キャプチャ画面の決定
	//------------------------------------------------------------------------------------
	GetScreenSize();

	//------------------------------------------------------------------------------------
	//	リソースの作成
	//------------------------------------------------------------------------------------

	// デバイスリソースの作成
	g_deviceResources = std::make_shared<DeviceResources>();

	// キャプチャ用リソースの取得
//	GetDuplications();

	// ウィンドウ登録 & ウィンドウサイズに依存するリソースの作成
	for (UINT i = 0; i < g_WinHandle.size(); i++)
		g_deviceResources->RegisterWindow(g_WinHandle[i]);		// ⇒ ウィンドウの数だけスワップチェーンを作成

	// アプリケーションリソース
	g_mainResources = std::make_shared<MainResources>(g_deviceResources);

	// シーンレンダラー
	g_sceneRenderer = std::unique_ptr<SceneRenderer>(new SceneRenderer(g_deviceResources, g_mainResources));

	//------------------------------------------------------------------------------------
	//	アプリケーションループ
	//------------------------------------------------------------------------------------
	AppLoop();

	//------------------------------------------------------------------------------------
	//	リソース解放
	//------------------------------------------------------------------------------------
	Cleanup();


	return 0;
}


//====================================================================================================
//	リソースの解放
//====================================================================================================
void Cleanup()
{

}

//====================================================================================================
//	メッセージ・プロシージャ
//====================================================================================================
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg) {

		case
			WM_KEYDOWN:
				switch (wParam) {

		//		case 'Q':		g_mainResources->StartLifeGame();			break;
		//		case 'R':		g_mainResources->InitLifeGame();			break;
		//		case 'S':		g_mainResources->SetpLifeGame();			break;

				case '0':		g_mainResources->InitLifeGame(0);			break;
				case '1':		g_mainResources->InitLifeGame(1);			break;

		//		case VK_UP:		g_mainResources->RateFloat(1.01f);			break;
		//		case VK_DOWN:	g_mainResources->RateFloat(0.99f);			break;
		//		case VK_PRIOR:	g_mainResources->AddFloat(+0.1f);			break;
		//		case VK_NEXT:	g_mainResources->AddFloat(-0.1f);			break;


				}

		case WM_SYSKEYDOWN:
			if (VK_ESCAPE == wParam) {
				PostQuitMessage(0);			// アプリケーションループ終了
			}
			return 0;

		case WM_TOUCH:
			TouchEventProc((HTOUCHINPUT)lParam, (UINT)wParam);
			break;

			// マウス
		case WM_MOUSEMOVE:					// マウスが移動したとき
		case WM_LBUTTONDOWN:				// 左ボタンが押されたとき
		case WM_LBUTTONUP:					// 左ボタンが離されたとき
			MouseEventProc(uMsg, wParam);
			break;

		case WM_RBUTTONDOWN:				// 右ボタンが押されたとき
		case WM_RBUTTONUP:					// 右ボタンが離されたとき
			MouseEventProc(uMsg, wParam);
			break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//====================================================================================================
//	アプリケーションループ
//====================================================================================================
HRESULT AppLoop()
{
	HRESULT	hr = S_OK;

	//------------------------------------------------------------
	// メッセージループ
	//------------------------------------------------------------
	bool bGotMsg;
	MSG msg;
	msg.message = WM_NULL;
	PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);

	HWND hWnd = g_WinHandle[0];

	while (WM_QUIT != msg.message) {

		// メッセージのチェック 
		bGotMsg = (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0);

		if (bGotMsg) {
			if (0 == TranslateAccelerator(hWnd, NULL, &msg)) {
				DispatchMessage(&msg);			// プロシージャに送出
			}
		}

		else {
			Update();
			Render();
		}
	}

	return hr;
}

//====================================================================================================
//	【アダプタ(グラフィックカード)の出力環境の調査】
//====================================================================================================
HRESULT CheckAdapter()
{
	HRESULT hr = S_OK;

	UINT	NumAdapters = 0;		// Adapter(グラボ)数 

	Microsoft::WRL::ComPtr<IDXGIFactory2> pDXGIFactory;

	// DXGI Factoryインターフェイスの作成
	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pDXGIFactory);
	if (FAILED(hr))	return hr;

	// Adapter(グラボ)を列挙
	for (UINT i = 0;; i++) {

		IDXGIAdapter1 *pAdapter = NULL;

		hr = pDXGIFactory->EnumAdapters1(i, &pAdapter);
		if (DXGI_ERROR_NOT_FOUND == hr)	break;
		if (FAILED(hr))					break;

		NumAdapters++;

		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

		// 使用可能なAdapterが列挙された場合、使用可能なOutputを列挙
		for (UINT j = 0;; j++) {

			IDXGIOutput1* pOutput = NULL;
			hr = pAdapter->EnumOutputs(j, (IDXGIOutput**)&pOutput);

			if (DXGI_ERROR_NOT_FOUND == hr) break;
			if (FAILED(hr)) break;

			// 利用可能なOutputの記述を取得
			DXGI_OUTPUT_DESC desc;
			pOutput->GetDesc(&desc);
			if (!desc.AttachedToDesktop) { pOutput->Release(); continue; }

			// 有効な出力を検出
			g_pOutput.push_back(pOutput);

		}
	}

	// ここで環境のチェックを入れる
	if (g_pOutput.size()<1)						return E_FAIL;		// 表示するモニタがない

	return S_OK;
}

//====================================================================================================
//	【ウィンドウのセットアップ】
//		出力数に合わせてウィンドウを作成
//====================================================================================================
HRESULT SetupWindow(HINSTANCE hInstance)
{
	//---------------------------------------------------------------------------------
	// ウィンドウクラスの作成＆登録
	//---------------------------------------------------------------------------------
	WNDCLASS	WindowClass;									// 登録ウィンドウクラス
	WCHAR		szWindowClass[] = { L"Main" };					// 登録ウィンドウクラス名
	WCHAR		szWindowName[] = { L"Main" };					// ウィンドウ名(Windowed)
	WCHAR		szExePath[MAX_PATH];
	GetModuleFileName(NULL, szExePath, MAX_PATH);
	HICON hIcon = ExtractIcon(hInstance, szExePath, 0);

	ZeroMemory(&WindowClass, sizeof(WNDCLASS));
	WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WindowClass.hIcon = hIcon;
	WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	WindowClass.style = CS_DBLCLKS;
	WindowClass.lpfnWndProc = MsgProc;
	WindowClass.hInstance = hInstance;
	WindowClass.lpszClassName = szWindowClass;

	// 登録
	ATOM ClassAtom = RegisterClass(&WindowClass);
	if (ClassAtom == 0) {
		DWORD error = GetLastError();
		if (ERROR_CLASS_ALREADY_EXISTS != error) {
			MessageBox(NULL, L"ウィンドウクラスの登録に失敗しました。", L"Error", MB_OK | MB_ICONEXCLAMATION);
			return E_FAIL;
		}
	}

	//---------------------------------------------------------------------------------
	// ウィンドウの作成
	//---------------------------------------------------------------------------------
	UINT numOutput = 1; //g_pOutput.size();

	for (UINT i = 0; i<numOutput; i++) {
		DWORD dwStyle = WS_OVERLAPPEDWINDOW;

		// ウィンドウ縦横サイズ
		int W = WINDOW_WIDTH;
		int H = WINDOW_HEIGHT;

		// ウィンドウ表示位置
		DXGI_OUTPUT_DESC desc;
		int no = (i>numOutput - 1) ? numOutput - 1 : i;
		g_pOutput[no]->GetDesc(&desc);

		int X = desc.DesktopCoordinates.left;	// X座標 
		int Y = desc.DesktopCoordinates.top;	// Y座標

		HWND hWnd = CreateWindow(
			szWindowClass,					// 登録ウィンドウクラス名
			szWindowName,					// ウィンドウ名
			dwStyle,						// ウィンドウスタイル
			X, Y,							// ウィンドウ位置
			W, H,							// ウィンドウ縦横幅
			NULL,							// 親ウィンドウのハンドル
			0,								// メニューハンドル/子ウィンドウID
			WindowClass.hInstance,			// Appインスタンスのハンドル
			NULL);							// ウィンドウ作成データ

		g_WinHandle.push_back(hWnd);

		//---------------------------------------------------------------------------------
		// タッチジェスチャーの設定(ウィンドウ毎)
		//---------------------------------------------------------------------------------
		DWORD dwPanWant = GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY | GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;
		DWORD dwPanBlock = GC_PAN_WITH_GUTTER | GC_PAN_WITH_INERTIA;

		GESTURECONFIG gc[] = { { GID_ZOOM, GC_ZOOM, 0 },
		{ GID_ROTATE, GC_ROTATE, 0 },
		{ GID_PAN, dwPanWant, dwPanBlock }
		};

		UINT uiGcs = 3;
		BOOL result = SetGestureConfig(hWnd, 0, uiGcs, gc, sizeof(GESTURECONFIG));

		if (!result) {
			DWORD err = GetLastError();
		}

		//---------------------------------------------------------------------------------
		// タッチ入力(ウィンドウ毎)
		//---------------------------------------------------------------------------------
		RegisterTouchWindow(hWnd, TWF_WANTPALM);
	}

	//---------------------------------------------------------------------------------
	//	ウィンドウの表示
	//---------------------------------------------------------------------------------
	for (UINT i = 0; i < g_WinHandle.size(); i++) {
		ShowWindow(g_WinHandle[i], SW_SHOWDEFAULT);
		ValidateRect(g_WinHandle[i], 0);
	}

	return S_OK;
}

//======================================================================================
//	マウスメッセージの処理
//======================================================================================
void MouseEventProc(UINT uMsg, WPARAM wParam)
{

	// タッチによって生じたマウスイベントを無視する
#define MOUSEEVENTF_FROMTOUCH 0xFF515700
	if ((GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) == MOUSEEVENTF_FROMTOUCH) {
		return;
	}

	bool flgMouseDOWN = (WM_LBUTTONDOWN == uMsg) ? true : false;						// マウス左ボタンが押された
	bool flgMouseUP = (WM_LBUTTONUP == uMsg) ? true : false;							// マウスの左ボタンが離された
	bool flgMouseMOVE = (WM_MOUSEMOVE == uMsg && MK_LBUTTON == wParam) ? true : false;	// マウス左ボタンが押された状態で移動

																						// スクリーン座標でのカーソル位置を取得
	POINT	CursorPos;
	GetCursorPos(&CursorPos);

	// イベント処理
	UIEvent(g_hWndMouse, CursorPos, flgMouseDOWN, flgMouseUP, flgMouseMOVE);

}

//====================================================================================================
//	タッチによる動作
//====================================================================================================
void TouchEventProc(HTOUCHINPUT hTouchInput, UINT numInputs)
{
	if (!hTouchInput)	return;
	if (numInputs <= 0)	return;

	// タッチ情報の取得
	BOOL ret = GetTouchInputInfo(hTouchInput, numInputs, g_TouchInfo, sizeof(TOUCHINPUT));
	g_NumTouch = numInputs;

	// タッチイベント
	int no = numInputs - 1;		// ※マルチタッチのうち常に[0]しか見ていない
	bool flgTouchUp = (g_TouchInfo[no].dwFlags & TOUCHEVENTF_UP) ? true : false;
	bool flgTouchDown = (g_TouchInfo[no].dwFlags & TOUCHEVENTF_DOWN) ? true : false;
	bool flgTouchMove = (g_TouchInfo[no].dwFlags & TOUCHEVENTF_MOVE) ? true : false;

	// タッチ位置 (物理画面座標→スクリーン座標)
	POINT	CursorPos;
	CursorPos.x = g_TouchInfo[no].x / 100;
	CursorPos.y = g_TouchInfo[no].y / 100;

	// タッチ入力ハンドルを閉じる
	CloseTouchInputHandle(hTouchInput);

	// イベント処理
	UIEvent(g_hWndTouch, CursorPos, flgTouchDown, flgTouchUp, flgTouchMove);

}

//======================================================================================
//	マウス・タッチの処理
//======================================================================================
void UIEvent(HWND hWnd, POINT pCrnt, bool flgDown, bool flgUp, bool flgMove)
{

	//---------------------------------------------------------------------------
	//	移動の変化量
	//---------------------------------------------------------------------------	
	static POINT pPrev = pCrnt;
	LONG DifX = pCrnt.x - pPrev.x;
	LONG DifY = pCrnt.y - pPrev.y;
	pPrev = pCrnt;

	g_flgDown = flgMove;

	//---------------------------------------------------------------------------
	// クライアント領域での座標に変換 (pCrnt → mp)
	//---------------------------------------------------------------------------
	POINT mp = pCrnt;
	LONG x = mp.x;
	LONG y = mp.y;

	g_CursorPosOld = g_CursorPos;
	g_CursorPos = mp;


	//---------------------------------------------------------------------------
	// 前回の
	//---------------------------------------------------------------------------
	static bool flgPrevD = false;		// 前回のDownフラグの状態
	static bool flgPrevU = false;		// 前回のUpフラグの状態

	flgPrevD = flgDown;
	flgPrevU = flgUp;

}

//====================================================================================================
//	【更新】
//====================================================================================================
void Update()
{

	//-------------------------------------------------------------------------
	// シーン オブジェクトを更新
	//-------------------------------------------------------------------------
	g_mainResources->Update();
	g_sceneRenderer->Update();

}

//====================================================================================================
//	【レンダリング】	
//====================================================================================================
void Render()
{
	// 初回更新前にレンダリングは行わない
	if (flgFirstRendering) { flgFirstRendering = false; return; }

	auto context = g_deviceResources->GetD3DDeviceContext();

	context->ClearState();		// 全てのステートを初期化

	//---------------------------------------------------------------------------------
	// ウィンドウごとのレンダリング
	//---------------------------------------------------------------------------------
	for (UINT i = 0; i < g_WinHandle.size(); i++) {

		// 現在のレンダーターゲットを通知(DisplayNoを更新)
		g_deviceResources->SetRenderTarget(i);

		// ビューポートをリセットして全画面をターゲットにし、クリア
		SetupRenderTarget(i);

		// シーン描画
		g_sceneRenderer->Render();

		// ビューポートをリセットして全画面に
		auto viewport = g_deviceResources->GetViewport(i);
		context->RSSetViewports(1, &viewport);
		auto pRTV = g_deviceResources->GetRenderTargetView(i);
		auto pDSV = g_deviceResources->GetDepthStencilView(i);
		context->OMSetRenderTargets(1, &pRTV, pDSV);
		context->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	}

	//---------------------------------------------------------------------------------
	// Present　(バックバッファをフロントに表示)
	//---------------------------------------------------------------------------------
	int numWnd = g_WinHandle.size();
	for (UINT i = 0; i < g_WinHandle.size(); i++) {
		auto swapChain = g_deviceResources->GetSwapChain(i);
		swapChain->Present(0, 0);
	}

	return;

}

//======================================================================================
//	指定の出力(ディスプレイ)にレンダーターゲットを設定
//======================================================================================
void SetupRenderTarget(int no)
{
	auto context = g_deviceResources->GetD3DDeviceContext();


	// ビューポートをリセットして全画面をターゲットに
	auto viewport = g_deviceResources->GetViewport(no);				// ビューポート取得(全画面)
	auto pRTV = g_deviceResources->GetRenderTargetView(no);			// レンダーターゲット・ビュー(RTV)取得
	auto pDSV = g_deviceResources->GetDepthStencilView(no);			// デプスステンシル・ビュー(DSV)取得

																	//	float clear[4] = { 1, 1, 1, 1 };
																	//	float clear[4] = { 0, 0, 1, 1 };
	float clear[4] = { 0, 0, 0, 1 };

	context->RSSetViewports(1, &viewport);							// ビューポートを設定
	context->OMSetRenderTargets(1, &pRTV, pDSV);					// RTV・DSVを設定
	context->ClearRenderTargetView(pRTV, clear);					// RTVを指定の色でクリア
	context->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);	// DSVをクリア

}

//======================================================================================
//	クライアント領域を指定のサイズにするようにウィンドウサイズを変更
//	 (通常はタイトルバーなども含めて指定サイズになっているため)
//======================================================================================
inline void SetClientSize(HWND hWnd, int width, int height)
{
	RECT wRect, cRect;
	int W_WIDTH, W_HEIGET;					// ウィンドウの幅と高さを入れる変数
	int C_WIDTH, C_HEIGET;					// クライアントの幅と高さを入れる変数
	int S_WIDTH, S_HEIGET;					// ウィンドウの外枠の幅と高さを入れる変数
	int M_WIDTH, M_HEIGET;					// 指定したウィンドウのサイズを入れる変数

	GetWindowRect(hWnd, &wRect);			// ウィンドウ全体の左上座標と左下座標を求める
	W_WIDTH = wRect.right - wRect.left;		// ウィンドウの幅を求める
	W_HEIGET = wRect.bottom - wRect.top;	// ウィンドウの高さを求める

	GetClientRect(hWnd, &cRect);			// クライアントの左上座標と左下座標を求める
	C_WIDTH = cRect.right - cRect.left;		// クライアントの幅を求める
	C_HEIGET = cRect.bottom - cRect.top;	// クライアントの高さを求める

	S_WIDTH = W_WIDTH - C_WIDTH;
	S_HEIGET = W_HEIGET - C_HEIGET;

	M_WIDTH = width + S_WIDTH;
	M_HEIGET = height + S_HEIGET;

	// 計算した幅と高さをウィンドウに設定
	SetWindowPos(hWnd, HWND_TOP, 0, 0, M_WIDTH, M_HEIGET, SWP_NOMOVE);

}

//========================================================================================================
//	ディスプレイサイズの取得
//========================================================================================================
void	GetScreenSize()
{

	HWND desktop;
	RECT rc;
	static int width, height;

	//スクリーンの情報を得る
	desktop = GetDesktopWindow();
	GetWindowRect(desktop, &rc);
	width = rc.right;
	height = rc.bottom;

	g_WindowWidth = width;
	g_WindowHeight = height;

}