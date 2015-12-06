//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "JsWrapper.h"
#include <string>

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

class Console : public JsWrapper::IConsole
{
public:
	Console(TextBox^ pTextBox, MainPage^ pMainPage) : m_pTextBody(pTextBox), m_pMainPage(pMainPage) { }
	void Append(const std::wstring message) override
	{
		m_pTextBody->Text = m_pTextBody->Text + L"\n" + ref new String(message.c_str());
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

		m_pTextBody->Background = ref new SolidColorBrush(color);
	}

private:
	TextBox^ m_pTextBody;
	JsExec::MainPage^ m_pMainPage;
};

MainPage::MainPage()
{
	InitializeComponent();

	JsWrapper::IJsWrapper& wrapper = JsWrapper::Instance();
	
	auto psConsole = std::make_shared<Console>(ConsoleOutput, this);
	wrapper.SetConsole(psConsole);
}

void JsExec::MainPage::Execute()
{
	JsWrapper::IJsWrapper& wrapper = JsWrapper::Instance();

	String^ pCodeInput = CodeInput->Text;
	std::wstring codeInput(pCodeInput->Data());

	CodeInput->BorderBrush = ref new SolidColorBrush(Windows::UI::Colors::Black);

	try
	{
		wrapper.Execute(codeInput);
	}
	catch (JsWrapper::Exception::Script& scriptException)
	{
		ConsoleOutput->Text = ConsoleOutput->Text + L"\nException:\n" + ref new String(scriptException.why().c_str());
		CodeInput->BorderBrush = ref new SolidColorBrush(Windows::UI::Colors::Red);
	}
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
	{
		Execute();
	}
}


void JsExec::MainPage::resetButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	ConsoleOutput->Text = L"";
}
