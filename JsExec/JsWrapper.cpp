#include "pch.h"
#include "JsWrapper.h"

#define USE_EDGEMODE_JSRT
#include<jsrt.h>

#include<assert.h>

#define ThrowIfFalse(x) do { bool res = x; if (!res) { __debugbreak(); throw std::runtime_error("Assertion Failure: #x"); } } while(false);
#define ThrowIfFailed(x) do { JsErrorCode jsLastError = x; if (jsLastError != JsNoError) { __debugbreak(); throw std::runtime_error("API Failure: #x"); } } while(false);
#define Assert(x) do { JsErrorCode jsLastError = x; assert(jsLastError == JsNoError); } while(false);

using JsWrapper::IExecutionContext;

struct GlobalFunctions
{
	struct FunctionDefinition
	{
		const wchar_t* wzName;
		JsNativeFunction function;
		const wchar_t* wzHelpText;
		FunctionDefinition(const wchar_t* wzN, JsNativeFunction fn, const wchar_t* wzH) : wzName(wzN), function(fn), wzHelpText(wzH) {}
	};

	// We can't throw exceptions back to the JS API so add this layer of protection
	static JsValueRef SafeAPI(const wchar_t* wzName, _In_ JsValueRef callee, _In_ bool isConstructCall, _In_ JsValueRef *arguments, _In_ unsigned short argumentCount, _In_opt_ void* callbackState, std::function<void(IExecutionContext&)> fn)
	{
		if (!callbackState)
		{
			assert(false);
			return nullptr;
		}

		JsWrapper::IExecutionContext& executionContext = *static_cast<JsWrapper::IExecutionContext*>(callbackState);

		try
		{
			fn(executionContext);
		}
		catch (...)
		{
			executionContext.Console().Append(std::wstring(wzName) + L"failed");
		}

		return nullptr;
	}

	static JsValueRef CALLBACK Foobar(_In_ JsValueRef callee, _In_ bool isConstructCall, _In_ JsValueRef *arguments, _In_ unsigned short argumentCount, _In_opt_ void* callbackState)
	{
		return SafeAPI(L"foobar", callee, isConstructCall, arguments, argumentCount, callbackState, [] (IExecutionContext& executionContext) {
			std::wstring message = L"Hello World";
			executionContext.Console().Append(message);
		});
	}

	static JsValueRef CALLBACK ConsoleLog(_In_ JsValueRef callee, _In_ bool isConstructCall, _In_ JsValueRef *arguments, _In_ unsigned short argumentCount, _In_opt_ void* callbackState)
	{
		return SafeAPI(L"console_log", callee, isConstructCall, arguments, argumentCount, callbackState, [argumentCount, arguments] (IExecutionContext& executionContext) {
			for (unsigned int i = 1; i < argumentCount; i++)
			{
				JsValueRef stringValue;
				ThrowIfFailed(JsConvertValueToString(arguments[i], &stringValue));

				const wchar_t *wzString;
				size_t length;
				ThrowIfFailed(JsStringToPointer(stringValue, &wzString, &length));

				std::wstring message(wzString);
				executionContext.Console().Append(message);
			}
		});
	}

	static JsValueRef CALLBACK Sleep(_In_ JsValueRef callee, _In_ bool isConstructCall, _In_ JsValueRef *arguments, _In_ unsigned short argumentCount, _In_opt_ void* callbackState)
	{
		return SafeAPI(L"sleep", callee, isConstructCall, arguments, argumentCount, callbackState, [argumentCount, arguments] (IExecutionContext& executionContext) {
			ThrowIfFalse(argumentCount == 2);

			JsValueRef numberVal;
			ThrowIfFailed(JsConvertValueToNumber(arguments[1], &numberVal));
			int milliseconds;

			ThrowIfFailed(JsNumberToInt(numberVal, &milliseconds));

			std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
		});
	}

	static JsValueRef CALLBACK SetColor(_In_ JsValueRef callee, _In_ bool isConstructCall, _In_ JsValueRef *arguments, _In_ unsigned short argumentCount, _In_opt_ void* callbackState) noexcept
	{
		return SafeAPI(L"set_color", callee, isConstructCall, arguments, argumentCount, callbackState, [argumentCount, arguments] (IExecutionContext& executionContext) {
			ThrowIfFalse(argumentCount == 2);

			JsValueRef stringValue;
			ThrowIfFailed(JsConvertValueToString(arguments[1], &stringValue));

			const wchar_t *wzString;
			size_t length;
			ThrowIfFailed(JsStringToPointer(stringValue, &wzString, &length));

			std::wstring color(wzString);
			executionContext.Console().SetColor(color);
		});
	}

	
	static std::vector<double> ExtractNumbers(JsValueRef* arguments, unsigned short count)
	{
		std::vector<double> numbers(count);

		for (int i = 0; i < count; i++)
		{
			JsValueRef valRef;
			ThrowIfFailed(JsConvertValueToNumber(arguments[i], &valRef));
			double val;
			ThrowIfFailed(JsNumberToDouble(valRef, &val));
			numbers[i] = val;
		}

		return numbers;
	}

	static JsValueRef CALLBACK SetRotation(_In_ JsValueRef callee, _In_ bool isConstructCall, _In_ JsValueRef *arguments, _In_ unsigned short argumentCount, _In_opt_ void* callbackState)
	{
		return SafeAPI(L"set_rotation", callee, isConstructCall, arguments, argumentCount, callbackState, [argumentCount, arguments] (IExecutionContext& executionContext) {
			ThrowIfFalse(argumentCount == 4);

			std::vector<double> cords = ExtractNumbers(&arguments[1], argumentCount - 1);
			assert(cords.size() == 3);

			executionContext.Console().Rotate(cords[0], cords[1], cords[2]);
		});
	}

	static JsValueRef CALLBACK Help(_In_ JsValueRef callee, _In_ bool isConstructCall, _In_ JsValueRef *arguments, _In_ unsigned short argumentCount, _In_opt_ void* callbackState)
	{
		return SafeAPI(L"help", callee, isConstructCall, arguments, argumentCount, callbackState, [] (IExecutionContext& executionContext) {
			executionContext.Console().Append(L"welcome to jsexec\n i speak javascript below\nspecial commands:\n");

			for (auto& cmd : GetFunctions())
				executionContext.Console().Append(L"- " + std::wstring(cmd.wzName) + L": " + cmd.wzHelpText);
		});
	}

	static const std::vector<FunctionDefinition>& GetFunctions()
	{
		// TODO: This registration should be easier. Need to think of a cleaner way besides code generation.
		static std::vector<FunctionDefinition> functions {
			{ L"foobar", &Foobar, L"hello world method" }, 
			{ L"console_log", &ConsoleLog, L"append to console: console_log(\"Message\")" }, 
			{ L"sleep", &Sleep, L"sleep for n milliseconds: sleep(100)" }, 
			{ L"set_color", &SetColor, L"set console color (in hex): set_color(\"#AARRGGBB\")" },
			{ L"set_rotation", &SetRotation, L"set the console rotation: set_rotation(100, 200, -360)" },
			{ L"help", &Help, L"you found it" },
		};
		return functions;
	}
};

namespace JsWrapper
{

class ChakraExecutionContext : public IExecutionContext
{
public:
	ChakraExecutionContext(std::unique_ptr<IConsole>&& psConsole) : m_psConsole(std::move(psConsole)) {}
	IConsole& Console() override { ThrowIfFalse(m_psConsole != nullptr); return *m_psConsole; }

private:
	int m_value { 0 };
	std::unique_ptr<IConsole> m_psConsole;
};

class ChakraWrapper : public IJsWrapper
{
public:
	ChakraWrapper(std::unique_ptr<IConsole>&& psConsole);
	~ChakraWrapper();

	void Execute(const std::wstring code) override;
	IExecutionContext& ExecutionContext() { return m_executionContext; }

private:
	void RegisterGlobalFunction(const wchar_t* wzName, JsNativeFunction function);
	void GetAndThrowException();

	JsRuntimeHandle m_pJsRuntimeHandle { nullptr };
	JsContextRef m_pJsContext { nullptr };
	JsValueRef m_result;
	ChakraExecutionContext m_executionContext;
};

std::unique_ptr<IJsWrapper> CreateInstance(std::unique_ptr<IConsole>&& psConsole)
{
	return std::make_unique<ChakraWrapper>(std::move(psConsole));
}

ChakraWrapper::ChakraWrapper(std::unique_ptr<IConsole>&& psConsole) : m_executionContext(std::move(psConsole))
{
	// Initialize JS engine
	ThrowIfFailed(JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &m_pJsRuntimeHandle));

	// Create & set an execution context
	ThrowIfFailed(JsCreateContext(m_pJsRuntimeHandle, &m_pJsContext));
	ThrowIfFailed(JsSetCurrentContext(m_pJsContext));

	// Register function(s)
	for (auto& fn : GlobalFunctions::GetFunctions())
	{
		RegisterGlobalFunction(fn.wzName, fn.function);
	}
}

void ChakraWrapper::RegisterGlobalFunction(const wchar_t* wzName, JsNativeFunction function)
{
	JsValueRef global;
	ThrowIfFailed(JsGetGlobalObject(&global));

	JsValueRef jsFunc;
	ThrowIfFailed(JsCreateFunction(function, &ExecutionContext(), &jsFunc));

	JsPropertyIdRef funcNameProp;
	ThrowIfFailed(JsGetPropertyIdFromName(wzName, &funcNameProp));

	ThrowIfFailed(JsSetProperty(global, funcNameProp, jsFunc, true));
}

ChakraWrapper::~ChakraWrapper()
{
	Assert(JsSetCurrentContext(JS_INVALID_REFERENCE));
	Assert(JsDisposeRuntime(m_pJsRuntimeHandle));
}

void ChakraWrapper::Execute(const std::wstring code)
{
	JsSourceContext sourceContext = 0;
	JsErrorCode scriptError = JsRunScript(code.c_str(), sourceContext, L"", &m_result);
	
	if (scriptError == JsNoError)
		return;
	
	ThrowIfFalse(scriptError == JsErrorScriptException || scriptError == JsErrorScriptCompile);

	GetAndThrowException();
}

void ChakraWrapper::GetAndThrowException()
{
	JsValueRef exception;
	ThrowIfFailed(JsGetAndClearException(&exception));

	JsPropertyIdRef messageName;
	ThrowIfFailed(JsGetPropertyIdFromName(L"message", &messageName));

	JsValueRef messageValue;
	ThrowIfFailed(JsGetProperty(exception, messageName, &messageValue));

	const wchar_t *wzMessage;
	size_t length;
	ThrowIfFailed(JsStringToPointer(messageValue, &wzMessage, &length));

	throw JsWrapper::Exception::Script(wzMessage);
}

}