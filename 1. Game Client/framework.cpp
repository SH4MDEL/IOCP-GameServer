#include "framework.h"
#include "scene.h"
#include "mainScene.h"

template<typename T>
T GetUserDataPtr(HWND hWnd)
{
	return reinterpret_cast<T>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
}
void SetUserDataPtr(HWND hWnd, LPVOID ptr)
{
	LONG_PTR result = ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ptr));
}

Framework::Framework() : m_hInstance{nullptr}, m_hWnd{nullptr},
	m_rcClient{0, 0, 0, 0}, m_hBitmapBackBuffer{nullptr}, m_hDC{nullptr},
	m_clrBackBuffer{ 0x00000000 }, m_hbrBackground{nullptr} {}

bool Framework::OnCreate(HINSTANCE hInstance, HWND hWnd, const RECT& rc)
{
	m_hInstance = hInstance;
	m_hWnd = hWnd;
	m_rcClient = rc;
	// Ŭ���̾�Ʈ ��ǥ �ʱ�ȭ
	m_rcClient.right -= m_rcClient.left;
	m_rcClient.bottom -= m_rcClient.top;
	m_rcClient.left = 0;
	m_rcClient.top = 0;

	// ���� ����
	CreatebackBuffer();

	// Ŭ������ ������ ���ν��� ����
	::SetUserDataPtr(m_hWnd, this);

	// ...

	// ĸ�� ����
	lstrcpy(m_captionTitle, TITLESTRING);

#if defined(SHOW_CAPTIONFPS)
	lstrcat(m_captionTitle, TEXT(" ("));
#endif
	m_titleLength = lstrlen(m_captionTitle);
	SetWindowText(m_hWnd, m_captionTitle);

	// �� ����
	BuildObjects();
	m_sceneIndex = Scene::Tag::Main;

	return m_hWnd != nullptr;
}

bool Framework::OnDestroy()
{
	return false;
}

void Framework::CreatebackBuffer()
{
	if (m_hDC)
	{
		::SelectObject(m_hDC, NULL);
		::DeleteDC(m_hDC);
	}
	if (m_hBitmapBackBuffer) ::DeleteObject(m_hBitmapBackBuffer);

	HDC hdc = ::GetDC(m_hWnd);
	m_hDC = ::CreateCompatibleDC(hdc);	// ȣȯ�� �����ִ� �Լ�
	// ������ ��ũ ���ο� ����۸� �����. hdc�� ȣȯ�� ��Ʈ���� ����Ѵ�.
	m_hBitmapBackBuffer = ::CreateCompatibleBitmap(hdc, m_rcClient.right, m_rcClient.bottom);
	::SelectObject(m_hDC, m_hBitmapBackBuffer);

	SetBKColor(RGB(255, 255, 255));

	::FillRect(m_hDC, &m_rcClient, m_hbrBackground);

	::ReleaseDC(m_hWnd, hdc);
}

void Framework::BuildObjects()
{
	m_timer.Tick();
	m_scenes[Scene::Tag::Main] = make_unique<MainScene>(Scene::Tag::Main);
	m_scenes[Scene::Tag::Main]->BuildObjects();
}

void Framework::FrameAdvance()
{
	m_timer.Tick();
	if (m_timer.GetFPS() > MAX_FPS) {
		Update(m_timer.GetDeltaTime());
		Render();

		PAINTSTRUCT ps;
		HDC hdc = ::BeginPaint(m_hWnd, &ps);
		::BitBlt(hdc, m_rcClient.left, m_rcClient.top, m_rcClient.right, m_rcClient.bottom, m_hDC, 0, 0, SRCCOPY);
		::EndPaint(m_hWnd, &ps);

		InvalidateRect(m_hWnd, &m_rcClient, false);
	}

	InvalidateRect(m_hWnd, &m_rcClient, false);

#if defined(SHOW_CAPTIONFPS)
	_itow_s(
		static_cast<int>(m_timer.GetFPS() + 0.1f)
		, m_captionTitle + m_titleLength
		, MAX_TITLE - m_titleLength
		, 10);
	wcscat_s(
		m_captionTitle + m_titleLength
		, MAX_TITLE - m_titleLength
		, TEXT("FPS)"));
	SetWindowText(m_hWnd, m_captionTitle);
#endif
}

LRESULT Framework::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Framework* self = ::GetUserDataPtr<Framework*>(hWnd);
	if (!self) return ::DefWindowProc(hWnd, message, wParam, lParam);

	switch (message)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		self->OnProcessingMouseMessage(hWnd, message, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
		self->OnProcessingKeyboardMessage(hWnd, message, wParam, lParam);
		break;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = ::BeginPaint(hWnd, &ps);
		::BitBlt(hdc, self->m_rcClient.left, self->m_rcClient.top, self->m_rcClient.right, self->m_rcClient.bottom, self->m_hDC, 0, 0, SRCCOPY);

		::EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		::SetUserDataPtr(hWnd, NULL);
		::PostQuitMessage(0);
		break;

	default:
		return self->OnProcessingWindowMessage(hWnd, message, wParam, lParam);
	}
	return 0;
	//	return LRESULT();
}

bool Framework::OnProcessingKeyboardMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam)
{
	switch (messageID)
	{
	//case WM_KEYDOWN:
	case WM_KEYUP:
		m_scenes[m_sceneIndex]->OnProcessingKeyboardMessage(hWnd, messageID, wParam, lParam);
		switch (wParam)
		{
		case VK_ESCAPE:
			DestroyWindow(hWnd);
			break;
		}
		return true;
	}
	return false;
}

bool Framework::OnProcessingMouseMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam)
{
	switch (messageID)
	{
	case WM_LBUTTONUP:
		break;
	case WM_LBUTTONDOWN:
		break;

	case WM_MOUSEMOVE:
		LOWORD(lParam);
		HIWORD(lParam);
		break;

	}
	return false;
}

HRESULT Framework::OnProcessingWindowMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam)
{
	switch (messageID)
	{
	case WM_SIZE:	// ������ ũ�⸦ �����ϴ� ���� ���� ���´�.
		break;
	default:
		return (HRESULT)::DefWindowProc(hWnd, messageID, wParam, lParam);
	}
	//	return E_NOTIMPL;
	return 0;
}

void Framework::Update(FLOAT timeElapsed)
{
	m_scenes[m_sceneIndex]->Update(timeElapsed);
}

void Framework::Render()
{
	::FillRect(m_hDC, &m_rcClient, m_hbrBackground);
	::SetBkColor(m_hDC, TRANSPARENT);	// ���ĺ��� ��� �����ϰ� ����
	::SetStretchBltMode(m_hDC, COLORONCOLOR);	// ���� ������ �޶� �þ�ų� �پ�� ������ �ִ� ��� �����.

	m_scenes[m_sceneIndex]->Render(m_hDC);
}

void Framework::SetBKColor(COLORREF color)
{
	m_clrBackBuffer = color;
	if (m_hbrBackground) ::DeleteObject(m_hbrBackground);
	m_hbrBackground = ::CreateSolidBrush(m_clrBackBuffer);
}
