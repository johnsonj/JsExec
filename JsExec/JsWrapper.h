#pragma once

namespace JsWrapper
{

class IExecutionContext;
class IConsole;

// Interface to the JavaScript engine for the host app.
// All interaction with the IJsWrapper must happen on a single thread.
class IJsWrapper
{
public:
	virtual ~IJsWrapper() {};
	virtual void Execute(const std::wstring code) = 0;
};

// Factory method for creating an IJsWrapper.
std::unique_ptr<IJsWrapper> CreateInstance(std::unique_ptr<IConsole>&& psConsole);


// Owned by the JavaScript runtime. Used to host any state needed for
// script execution.
class IExecutionContext
{
public:
	virtual ~IExecutionContext() {};
	virtual IConsole& Console() = 0;
};

// Callback site from the JavaScript runtime to the host.
// Calls will happen on the same thread as the JavaScript runtime is hosted.
// Callbacks block script execution.
class IConsole
{
public:
	virtual ~IConsole() {};
	virtual void Append(const std::wstring text) = 0;
	virtual void SetColor(const std::wstring hexColorStr) = 0;
	virtual void Rotate(double x, double y, double z) = 0;
};


namespace Exception
{
	class Script : public std::exception
	{
	public:
		// TODO: This _could_ throw, that's not very good.
		Script(const wchar_t* wzWhy) : m_why(wzWhy) { }
		const std::wstring& why() { return m_why;  }

	private:
		std::wstring m_why;
	};
}

}