﻿//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "JsWrapper.h"
#include <string>

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

		// #AARRGGBB or #RRGGBB is expected
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
		IAsyncAction^ pAction = m_pDispatcher->RunAsync(
			CoreDispatcherPriority::High,
			ref new DispatchedHandler([this, x, y, z]()
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
	InitializeComponent();
	CoreDispatcher^ pDispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;
	m_psConsole = std::make_shared<Console>(ConsoleOutput, this, pDispatcher);

	auto initJs = ref new WorkItemHandler([this, pDispatcher] (IAsyncAction^ workItem)
	{
		JsWrapper::IJsWrapper& wrapper = JsWrapper::Instance();

		wrapper.SetConsole(m_psConsole);
	});

	ThreadPool::RunAsync(initJs);
}

void JsExec::MainPage::Execute()
{
	String^ pCodeInput = CodeInput->Text;
	std::wstring codeInput(pCodeInput->Data());

	CodeInput->BorderBrush = ref new SolidColorBrush(Windows::UI::Colors::Black);

	auto workItem = ref new WorkItemHandler([this, codeInput](IAsyncAction^ workItem)
	{
		try
		{
			JsWrapper::Instance().Execute(codeInput);
		}
		catch (JsWrapper::Exception::Script& scriptException)
		{
			m_psConsole->Append(L"\nException:\n" + scriptException.why());
		}
	});

	ThreadPool::RunAsync(workItem);
}

void JsExec::MainPage::button_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Execute();
}

void JsExec::MainPage::CodeInput_TextChanged_1(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
}


void JsExec::MainPage::CodeInput_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
	if (e->Key == Windows::System::VirtualKey::F1)
		Execute();
	else if (e->Key == Windows::System::VirtualKey::F2)
		Reset();
}

void JsExec::MainPage::Reset()
{
	ConsoleOutput->Text = L"";
	ConsoleOutput->Background = ref new SolidColorBrush(Windows::UI::Colors::Black);
	ConsoleOutput->Projection = nullptr;
	CodeInput->Text = L"";
}

void JsExec::MainPage::resetButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Reset();
}
