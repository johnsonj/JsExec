#pragma once

namespace JsWrapper
{
	class IExecutionContext;
	class IConsole;

	class IJsWrapper
	{
	public:
		virtual ~IJsWrapper() = default;
		virtual void Execute(const std::wstring code) = 0;
		virtual IExecutionContext& GetExecutionContext() = 0;
		// TODO: Better way to pass this through
		virtual void SetConsole(std::shared_ptr<IConsole> psConsole) = 0;
	};

	IJsWrapper& Instance();


	// Maybe a site where we can store something interesting?
	class IExecutionContext
	{
	public:
		virtual ~IExecutionContext() = default;
		virtual void Set(int) = 0;
		virtual int Get() = 0;
		virtual IConsole& GetConsole() = 0;
	};

	class IConsole
	{
	public:
		virtual ~IConsole() = default;
		virtual void Append(const std::wstring text) = 0;
		virtual void SetColor(const std::wstring hexColorStr) = 0;
	};


	namespace Exception
	{
		class Script : public std::exception
		{
		public:
			// This _could_ throw, that's not very good.
			Script(const wchar_t* wzWhy) : m_why(wzWhy) { }
			const std::wstring& why() { return m_why;  }

		private:
			std::wstring m_why;
		};
	}
}