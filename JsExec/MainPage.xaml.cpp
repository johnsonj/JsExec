//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "JsWrapper.h"
#include <string>
#include <functional>
#include <ppltasks.h>

using namespace JsExec;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace concurrency;
using namespace Windows::Devices::Enumeration;
using namespace Windows::System::Threading;
using namespace Windows::UI::Core;

class Console : public JsWrapper::IConsole
{
public:
	Console(TextBox^ pTextBox, MainPage^ pMainPage, CoreDispatcher^ pDispatcher) : m_pTextBody(pTextBox), m_pMainPage(pMainPage), m_pDispatcher(pDispatcher) { }
	void Append(const std::wstring message) override
	{
		m_pDispatcher->RunAsync(
			CoreDispatcherPriority::High,
			ref new DispatchedHandler([this, message]()
		{
			m_pTextBody->Text = m_pTextBody->Text + L"\n" + ref new String(message.c_str());
		}));
	}

	void SetColor(const std::wstring userHexColorStr) override
	{
		// #AARRGGBB or #RRGGBB is expected
		std::wstring hexColorStr;

		// Remove leading # if it's there
		if (userHexColorStr[0] == L'#')
		{
			hexColorStr = userHexColorStr.substr(1, userHexColorStr.length() - 2);
		}
		else
		{
			hexColorStr = userHexColorStr;
		}

		Windows::UI::Color color;
		color.A = 255;
		int parsed = 0;
		if (hexColorStr.length() == 8)
		{
			parsed = 2;
			color.A = std::stoi(hexColorStr.substr(0,2), 0, 16);
		}
		color.R = std::stoi(hexColorStr.substr(parsed, 2), 0, 16);
		parsed += 2;
		color.G = std::stoi(hexColorStr.substr(parsed, 2), 0, 16);
		parsed += 2;
		color.B = std::stoi(hexColorStr.substr(parsed, 2), 0, 16);

		m_pDispatcher->RunAsync(
			CoreDispatcherPriority::High,
			ref new DispatchedHandler([this, color]()
		{
			m_pTextBody->Background = ref new SolidColorBrush(color);
		}));
	}

	void Rotate(double x, double y, double z)
	{
		IAsyncAction^ pAction = m_pDispatcher->RunAsync(CoreDispatcherPriority::High,ref new DispatchedHandler([this, x, y, z]()
		{
			PlaneProjection^ pPlaneProjection = ref new PlaneProjection();
			pPlaneProjection->RotationX = x;
			pPlaneProjection->RotationY = y;
			pPlaneProjection->RotationY = z;
			m_pTextBody->Projection = pPlaneProjection;
		}));
	}

private:
	TextBox^ m_pTextBody;
	JsExec::MainPage^ m_pMainPage;
	CoreDispatcher^ m_pDispatcher;
};

MainPage::MainPage()
{
	using JsWrapper::IConsole;

	InitializeComponent();
	CoreDispatcher^ pDispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;

	// Oh my, oh my, a shared_ptr to a unique_ptr? What kind of maddness is this?
	// There's no R value capture in C++ 11, so a lambda can't take ownership of a unique_ptr.
	// C++14 will have generalized capture making this possible without the maddness.
	auto pConsoleWrapper = std::make_shared<std::unique_ptr<IConsole>>(std::make_unique<Console>(ConsoleOutput, this, pDispatcher));
	ThreadPool::RunAsync(ref new WorkItemHandler([this, pConsoleWrapper](IAsyncAction^ workItem)
	{
		m_pWrapper = JsWrapper::CreateInstance(std::move(*pConsoleWrapper.get()));
	}));
}

void JsExec::MainPage::Execute()
{
	String^ pCodeInput = CodeInput->Text;
	std::wstring codeInput(pCodeInput->Data());
	CoreDispatcher^ pUIThreadDispatch = CoreWindow::GetForCurrentThread()->Dispatcher;

	ThreadPool::RunAsync(ref new WorkItemHandler([this, codeInput, pUIThreadDispatch](IAsyncAction^ workItem)
	{
		try
		{
			m_pWrapper->Execute(codeInput);
		}
		catch (JsWrapper::Exception::Script& scriptException)
		{
			std::wstring why = scriptException.why();
			pUIThreadDispatch->RunAsync(CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
			{
				ConsoleOutput->Text = ConsoleOutput->Text + L"\n" + L"Exception:\n" + ref new String(why.c_str());
			}));
		}
	}));
}

void JsExec::MainPage::runButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Execute();
}

void JsExec::MainPage::CodeInput_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
	// Secret hot keys
	if (e->Key == Windows::System::VirtualKey::F1)
		Execute();
	else if (e->Key == Windows::System::VirtualKey::F2)
		Reset();
}

void JsExec::MainPage::Reset()
{
	ConsoleOutput->Text = L"";
	ConsoleOutput->Background = nullptr;
	ConsoleOutput->Projection = nullptr;
	CodeInput->Text = L"";
}

void JsExec::MainPage::resetButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Reset();
}
