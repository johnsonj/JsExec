#include "pch.h"
#include "JsWrapper.h"

#define USE_EDGEMODE_JSRT
#include<jsrt.h>

#include<assert.h>

#define AssertOrThrow(x) do { bool res = x; if (!res) { __debugbreak(); throw std::runtime_error("Assertion Failure: #x"); } } while(false);
#define AssertJsSuccess(x) do { JsErrorCode jsLastError = x; if (jsLastError != JsNoError) { __debugbreak(); throw std::runtime_error("API Failure: #x"); } } while(false);
#define AssertJsSuccessNoThrow(x) do { JsErrorCode jsLastError = x; assert(jsLastError == JsNoError); } while(false);

struct Callsite
{
	struct FunctionDefinition
	{
		const wchar_t* wzName;
		JsNativeFunction function;
		FunctionDefinition(const wchar_t* wzN, JsNativeFunction fn) : wzName(wzN), function(fn) {}
	};

	static _Ret_maybenull_ JsValueRef CALLBACK Foobar(_In_ JsValueRef callee,
		_In_ bool isConstructCall,
		_In_ JsValueRef *arguments,
		_In_ unsigned short argumentCount,
		_In_opt_ void* callbackState)
	{
		// We should always recieve an IExecutionContext so we'll bet the program on it
		assert(callbackState);
		JsWrapper::IExecutionContext& executionContext = *static_cast<JsWrapper::IExecutionContext*>(callbackState);

		executionContext.Set(100);

		std::wstring message = L"Hello World";
		executionContext.GetConsole().Append(message);

		return nullptr;
	}

	static _Ret_maybenull_ JsValueRef CALLBACK ConsoleLog(_In_ JsValueRef callee,
		_In_ bool isConstructCall,
		_In_ JsValueRef *arguments,
		_In_ unsigned short argumentCount,
		_In_opt_ void* callbackState)
	{
		assert(callbackState);
		JsWrapper::IExecutionContext& executionContext = *static_cast<JsWrapper::IExecutionContext*>(callbackState);

		for (unsigned int i = 1; i < argumentCount; i++)
		{
			JsValueRef stringValue;
			AssertJsSuccess(JsConvertValueToString(arguments[i], &stringValue));

			const wchar_t *wzString;
			size_t length;
			AssertJsSuccess(JsStringToPointer(stringValue, &wzString, &length));

			std::wstring message(wzString);
			executionContext.GetConsole().Append(message);
		}
		return nullptr;
	}

	static _Ret_maybenull_ JsValueRef CALLBACK Sleep(_In_ JsValueRef callee,
		_In_ bool isConstructCall,
		_In_ JsValueRef *arguments,
		_In_ unsigned short argumentCount,
		_In_opt_ void* callbackState)
	{
		assert(argumentCount == 2);

		JsValueRef numberVal;
		AssertJsSuccess(JsConvertValueToNumber(arguments[1], &numberVal));
		int milliseconds;
		AssertJsSuccess(JsNumberToInt(numberVal, &milliseconds));
		

		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));

		return nullptr;
	}

	static _Ret_maybenull_ JsValueRef CALLBACK SetColor(_In_ JsValueRef callee,
		_In_ bool isConstructCall,
		_In_ JsValueRef *arguments,
		_In_ unsigned short argumentCount,
		_In_opt_ void* callbackState) noexcept
	{
		assert(callbackState);
		JsWrapper::IExecutionContext& executionContext = *static_cast<JsWrapper::IExecutionContext*>(callbackState);

		try
		{
			assert(argumentCount == 2);
			if (argumentCount != 2)
				return nullptr;

			JsValueRef stringValue;
			AssertJsSuccess(JsConvertValueToString(arguments[1], &stringValue));

			const wchar_t *wzString;
			size_t length;
			AssertJsSuccess(JsStringToPointer(stringValue, &wzString, &length));

			std::wstring color(wzString);
			executionContext.GetConsole().SetColor(color);
		}
		catch(...)
		{
			executionContext.GetConsole().Append(L"set_color failed");
		}
		return nullptr;
	}

	static _Ret_maybenull_ JsValueRef CALLBACK Help(_In_ JsValueRef callee,
		_In_ bool isConstructCall,
		_In_ JsValueRef *arguments,
		_In_ unsigned short argumentCount,
		_In_opt_ void* callbackState)
	{
		JsWrapper::IExecutionContext& executionContext = *static_cast<JsWrapper::IExecutionContext*>(callbackState);
		JsWrapper::IConsole& console = executionContext.GetConsole();

		console.Append(L"welcome to jsexec\n i speak javascript below\nspecial commands:\n");
		for (auto& cmd : GetFunctions())
		{
			console.Append(L"- " + std::wstring(cmd.wzName));
		}
		return nullptr;
	}

	static const std::vector<FunctionDefinition>& GetFunctions()
	{
		static std::vector<FunctionDefinition> functions{ 
			{ L"foobar", &Foobar }, 
			{ L"console_log", &ConsoleLog }, 
			{ L"sleep", &Sleep }, 
			{ L"set_color", &SetColor },
			{ L"help", &Help },
		};
		return functions;
	}
};

namespace JsWrapper
{
	class ChakraWrapper;

	class ExecutionContext : public IExecutionContext
	{
	public:
		int Get() override { return m_value; }
		void Set(int val) override { m_value = val; }
		// TODO: Throw if m_psConsole null
		IConsole& GetConsole() override { return *m_psConsole; }
		void SetConsole(std::shared_ptr<IConsole> psConsole) { m_psConsole = psConsole; }

	private:
		int m_value { 0 };
		std::shared_ptr<IConsole> m_psConsole;
	};

	class ChakraWrapper : public IJsWrapper
	{
	public:
		ChakraWrapper();
		~ChakraWrapper() noexcept;

		void Execute(const std::wstring code) override;
		IExecutionContext& GetExecutionContext() override { return m_executionContext; }
		void SetConsole(std::shared_ptr<IConsole> psConsole) override { m_executionContext.SetConsole(psConsole); }

	private:
		void RegisterGlobalFunction(const wchar_t* wzName, JsNativeFunction function);
		void GetAndThrowException();

		JsRuntimeHandle m_pJsRuntimeHandle { nullptr };
		JsValueRef m_result;
		JsContextRef m_pJsContext { nullptr };
		ExecutionContext m_executionContext;
	};

	IJsWrapper& Instance()
	{
		static ChakraWrapper wrapper;
		return wrapper;
	};

	ChakraWrapper::ChakraWrapper()
	{
		// Initialize JS engine
		AssertJsSuccess(JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &m_pJsRuntimeHandle));

		// Create & set an execution context
		AssertJsSuccess(JsCreateContext(m_pJsRuntimeHandle, &m_pJsContext));
		AssertJsSuccess(JsSetCurrentContext(m_pJsContext));

		// Register function(s)
		for (auto& fn : Callsite::GetFunctions())
		{
			RegisterGlobalFunction(fn.wzName, fn.function);
		}
	}

	void ChakraWrapper::RegisterGlobalFunction(const wchar_t* wzName, JsNativeFunction function)
	{
		JsValueRef global;
		AssertJsSuccess(JsGetGlobalObject(&global));

		JsValueRef jsFunc;
		AssertJsSuccess(JsCreateFunction(function, &GetExecutionContext(), &jsFunc));

		JsPropertyIdRef funcNameProp;
		AssertJsSuccess(JsGetPropertyIdFromName(wzName, &funcNameProp));

		AssertJsSuccess(JsSetProperty(global, funcNameProp, jsFunc, true));
	}

	ChakraWrapper::~ChakraWrapper()
	{
		AssertJsSuccessNoThrow(JsSetCurrentContext(JS_INVALID_REFERENCE));
		AssertJsSuccessNoThrow(JsDisposeRuntime(m_pJsRuntimeHandle));
	}

	void ChakraWrapper::Execute(const std::wstring code)
	{
		JsSourceContext sourceContext = 0;
		JsErrorCode scriptError = JsRunScript(code.c_str(), sourceContext, L"", &m_result);
		
		if (scriptError == JsNoError)
			return;
		
		AssertOrThrow(scriptError == JsErrorScriptException || scriptError == JsErrorScriptCompile);

		GetAndThrowException();
	}

	void ChakraWrapper::GetAndThrowException()
	{
		JsValueRef exception;
		AssertJsSuccess(JsGetAndClearException(&exception));

		JsPropertyIdRef messageName;
		AssertJsSuccess(JsGetPropertyIdFromName(L"message", &messageName));

		JsValueRef messageValue;
		AssertJsSuccess(JsGetProperty(exception, messageName, &messageValue));

		const wchar_t *wzMessage;
		size_t length;
		AssertJsSuccess(JsStringToPointer(messageValue, &wzMessage, &length));

		throw JsWrapper::Exception::Script(wzMessage);
	}
}