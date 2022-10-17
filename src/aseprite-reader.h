/*
 * asepritereader.h
 *
 *  Created on: 21 kwi 2020, by crm
 *  Modified: oct 2022, by 56789a1987
 */

#pragma once
#define IS_NODE

#include <exception>
#include <memory>
#include <string>
#include <vector>

#ifdef IS_NODE
#include <napi.h>
#endif

class AsepriteReader final
{
public:
	enum class BlendMode
	{
		NORMAL = 0,

		DARKEN = 1,
		MULTIPLY = 2,
		COLOR_BURN = 3,

		LIGHTEN = 4,
		SCREEN = 5,
		COLOR_DODGE = 6,
		ADDITION = 7,

		OVERLAY = 8,
		SOFT_LIGHT = 9,
		HARD_LIGHT = 10,

		DIFFERENCE = 11,
		EXCLUSION = 12,
		SUBTRACT = 13,
		DIVIDE = 14,

		HUE = 15,
		SATURATION = 16,
		COLOR = 17,
		LUMINOSITY = 18,
	};

	enum class AnimationDirection
	{
		FORWARD = 0,
		REVERSE = 1,
		PINGPONG = 2,
	};

protected:
	struct Color
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	};

	class ResourceLoadException : public std::exception
	{
	protected:
		std::string message;

	public:
		ResourceLoadException(const std::string &message) : exception(), message(message) {}

		virtual const char *what() const noexcept override { return message.c_str(); }
	};

public:
	struct Frame;
	struct FrameTag;
	struct Palette;
	struct Layer;
	struct Cel;
	struct Slice;
	struct SliceKey;

	struct AsepriteFile
	{
		int width;
		int height;
		int numFrames;
		uint8_t colorDepth;
		uint16_t numColors;
		double pixelRatio;
		std::unique_ptr<Palette> palette = nullptr;

		std::vector<std::unique_ptr<Frame>> frames;
		std::vector<std::unique_ptr<FrameTag>> tags;
		std::vector<std::unique_ptr<Layer>> layers;
		std::vector<std::unique_ptr<Cel>> cels;
		std::vector<std::unique_ptr<Slice>> slices;
	};

	struct Frame
	{
		int duration = 0; // ms

		std::vector<Cel *> cels;
		std::vector<FrameTag *> tags;

#ifdef IS_NODE
		Napi::Object object;
		Napi::Array objCels;
		Napi::Array objTags;
#endif
	};

	struct FrameTag
	{
		std::string name;
		Color color;

		int frameFrom = 0;
		int frameTo = 0;

		AnimationDirection direction; // forward, reverse, pingpong

		std::vector<Frame *> frames;

#ifdef IS_NODE
		Napi::Object object;
		Napi::Array objFrames;
#endif
	};

	struct Palette
	{
		int paletteSize = 0;
		int firstColor = 0;
		int lastColor = 0;
		std::vector<std::unique_ptr<Color>> colors;

#ifdef IS_NODE
		Napi::Object object;
		Napi::Array objColors;
#endif
	};

	struct Layer
	{
		std::string name;

		uint16_t type;
		uint16_t flags;
		int opacity = 0; // 0-255
		BlendMode blendMode;

		Layer *layerParent = nullptr;
		std::vector<Layer *> layerChildren;

#ifdef IS_NODE
		Napi::Object object;
		Napi::Array objChildren;
#endif
	};

	struct Cel
	{
		int x;
		int y;
		int w;
		int h;
		int opacity = 0;
		int link = -1;

		std::shared_ptr<uint8_t> pixels;

		Frame *frame = nullptr;
		Layer *layer = nullptr;

#ifdef IS_NODE
		Napi::Object object;
		Napi::Uint8Array objPixels;
#endif
	};

	struct Slice
	{
		std::string name;
		bool has9Slice = false;
		bool hasPivot = false;

		std::vector<std::unique_ptr<SliceKey>> keys;

#ifdef IS_NODE
		Napi::Object object;
		Napi::Array objKeys;
#endif
	};

	struct SliceKey
	{
		int x;
		int y;
		int w;
		int h;
		int frame;

		int patchX;
		int patchY;
		int patchW;
		int patchH;

		int pivotX;
		int pivotY;
	};

public:
	AsepriteFile file;
#ifdef IS_NODE
	Napi::Object object;
#endif

public:
	AsepriteReader() = default;
	~AsepriteReader() = default;

#ifdef IS_NODE
	void load(const uint8_t *in, const uint32_t size, const Napi::CallbackInfo &info);
#else
	void load(const uint8_t *in, const uint32_t size);
#endif
};
