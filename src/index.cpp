#include <napi.h>
#include "aseprite-reader.h"

using namespace Napi;

Object ReadFile(const CallbackInfo &info)
{
	Env env = info.Env();
	Object EMPTY = Object::New(env);

	if (!info.Length() || !info[0].IsTypedArray())
	{
		Error::New(env, "Expected one Uint8Array argument").ThrowAsJavaScriptException();
		return EMPTY;
	}

	Uint8Array buffer = info[0].As<Uint8Array>();
	AsepriteReader reader;

	try
	{
		reader.load(buffer.Data(), buffer.ByteLength(), info);
	}
	catch (const std::exception &e)
	{
		Error::New(env, e.what()).ThrowAsJavaScriptException();
		return EMPTY;
	}

	return reader.object;
}

Object Init(Env env, Object exports)
{
	exports.Set(String::New(env, "AsepriteReader"), Function::New(env, ReadFile));
	return exports;
}

NODE_API_MODULE(AsepriteReader, Init)
