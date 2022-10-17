/*
 * asepritereader.cpp
 *
 *  Created on: 21 apr 2020, by crm
 *  Modified: oct 2022, by 56789a1987
 */

#include "aseprite-reader.h"

#include <map>
#include <zlib.h>

#ifdef IS_NODE

using namespace Napi;

// helpers

#define n_num(val) Number::New(env, val)
#define n_str(val) String::New(env, val)
#define newArray Array::New(env)
#define newObject Object::New(env)

#define obj_push(obj, val) obj[obj.Length()] = val
#define set_color(key, val)              \
	Array objColor = Array::New(env, 4); \
	objColor[0u] = n_num(val.r);         \
	objColor[1u] = n_num(val.g);         \
	objColor[2u] = n_num(val.b);         \
	objColor[3u] = n_num(val.a);         \
	key = objColor;

#endif

const uint16_t ASEPRITE_MAGIC_NUMBER_FILE = 0xA5E0;
const uint16_t ASEPRITE_MAGIC_NUMBER_FRAME = 0xF1FA;

enum ChunkType
{
	CHUNK_LAYER = 0x2004,
	CHUNK_CEL = 0x2005,
	CHUNK_CELEXTRA = 0x2006,
	CHUNK_FRAME_TAGS = 0x2018,
	CHUNK_PALETTE = 0x2019,
	CHUNK_USERDATA = 0x2020,
	CHUNK_SLICE = 0x2022,
};

enum LayerType
{
	LAYER_NORMAL = 0,
	LAYER_GROUP = 1,
};

enum Flag
{
	FLAG_USERDATA_TEXT = 0x1 << 0,
	FLAG_USERDATA_COLOR = 0x1 << 1,

	FLAG_SLICE_9SLICES = 0x1 << 0,
	FLAG_SLICE_PIVOT = 0x1 << 1,
};

enum CelType
{
	CEL_RAW = 0,
	CEL_LINKED = 1,
	CEL_COMPRESSED = 2,
};

#ifdef IS_NODE
void AsepriteReader::load(const uint8_t *in, const uint32_t size, const CallbackInfo &info)
#else
void AsepriteReader::load(const uint8_t *in, const uint32_t size)
#endif
{
	uint32_t ptr = 0;

	auto readUInt8 = [&ptr, &size](const uint8_t *in) -> uint8_t
	{
		uint8_t tmp = in[ptr];
		ptr++;
		if (ptr > size)
			throw ResourceLoadException("Unexpected EOF");
		return tmp;
	};
	auto readInt16 = [&ptr, &size](const uint8_t *in) -> int16_t
	{
		int16_t tmp = in[ptr] | (in[ptr + 1] << 8);
		ptr += 2;
		if (ptr > size)
			throw ResourceLoadException("Unexpected EOF");
		return tmp;
	};
	auto readUInt16 = [&ptr, &size](const uint8_t *in) -> uint16_t
	{
		uint16_t tmp = in[ptr] | (in[ptr + 1] << 8);
		ptr += 2;
		if (ptr > size)
			throw ResourceLoadException("Unexpected EOF");
		return tmp;
	};
	auto readUInt32 = [&ptr, &size](const uint8_t *in) -> uint32_t
	{
		uint32_t tmp = in[ptr] | (in[ptr + 1] << 8) | (in[ptr + 2] << 16) | (in[ptr + 3] << 24);
		ptr += 4;
		if (ptr > size)
			throw ResourceLoadException("Unexpected EOF");
		return tmp;
	};
	auto readString = [&ptr, &size](const uint8_t *in, unsigned short length) -> std::string
	{
		std::string buff;
		buff.resize(length); // Resize adds \0 at the end
		for (unsigned short i = 0; i < length; i++, ptr++)
		{
			buff[i] = in[ptr];
		}
		if (ptr > size)
			throw ResourceLoadException("Unexpected EOF");
		return buff;
	};
	auto readBytes = [&ptr, &size](const uint8_t *in, unsigned short length) -> uint8_t *
	{
		uint8_t *data = new uint8_t[length + 1];
		for (unsigned short i = 0; i < length; i++, ptr++)
		{
			data[i] = in[ptr];
		}
		data[length] = 0;
		if (ptr > size)
			throw ResourceLoadException("Unexpected EOF");
		return data;
	};
	auto skipBytes = [&ptr, &size](const uint8_t *in, int count) -> void
	{
		ptr += count;
		if (ptr > size)
			throw ResourceLoadException("Unexpected EOF");
	};

#ifdef IS_NODE
	Env env = info.Env();
	object = newObject;
#endif

	skipBytes(in, 4); // File size

	if (readUInt16(in) != ASEPRITE_MAGIC_NUMBER_FILE)
	{
		throw ResourceLoadException("Magic number mismatch");
	}

	const unsigned short FRAME_COUNT = readUInt16(in);
	file.width = readUInt16(in);
	file.height = readUInt16(in);
	file.numFrames = FRAME_COUNT;
	file.colorDepth = readUInt16(in);
	const unsigned short bytesPerPixel = file.colorDepth / 8;
	file.palette = std::make_unique<Palette>();

	skipBytes(in, 4 + 2 + 12); // File flags + Deprecated speed

	file.numColors = readUInt16(in);
	uint8_t pixelWidth = readUInt8(in);
	uint8_t pixelHeight = readUInt8(in);
	file.pixelRatio = pixelWidth && pixelHeight ? (double)pixelWidth / (double)pixelHeight : 1.0;

	skipBytes(in, 92);

#ifdef IS_NODE
	// general node object
	object["width"] = n_num(file.width);
	object["height"] = n_num(file.height);
	object["colorDepth"] = n_num(file.colorDepth);
	object["numFrames"] = n_num(FRAME_COUNT);
	object["numColors"] = n_num(file.numColors);
	object["pixelRatio"] = n_num(file.pixelRatio);

	// node object arrays and objects
	Array objFrames = newArray;
	Array objTags = newArray;
	Array objLayers = newArray;
	Array objCels = newArray;
	Array objSlices = newArray;
	Object objPalette = newObject;
	object["frames"] = objFrames;
	object["tags"] = objTags;
	object["layers"] = objLayers;
	object["cels"] = objCels;
	object["slices"] = objSlices;
	object["palette"] = objPalette;
#endif

	std::map<int, Layer *> layerIndex;

	for (unsigned idxFrame = 0u; idxFrame < FRAME_COUNT; ++idxFrame)
	{
		skipBytes(in, 4);

		if (readUInt16(in) != ASEPRITE_MAGIC_NUMBER_FRAME)
		{
			throw ResourceLoadException("Magic number mismatch");
		}

		file.frames.push_back(std::make_unique<Frame>());
		Frame *frame = file.frames.back().get();

		const unsigned short CHUNK_COUNT = readUInt16(in);
		frame->duration = readUInt16(in);
		skipBytes(in, 6);

#ifdef IS_NODE
		// frame node object
		frame->object = newObject;
		frame->objCels = newArray;
		frame->objTags = newArray;
		obj_push(objFrames, frame->object);
		frame->object["duration"] = n_num(frame->duration);
		frame->object["cels"] = frame->objCels;
		frame->object["tags"] = frame->objTags;
#endif

		for (unsigned idxChunk = 0u; idxChunk < CHUNK_COUNT; ++idxChunk)
		{
			const unsigned int CHUNK_SIZE = readUInt32(in);
			const unsigned short CHUNK_TYPE = readUInt16(in);

			switch (CHUNK_TYPE)
			{
			case CHUNK_LAYER:
			{
				file.layers.push_back(std::make_unique<Layer>());
				Layer *layer = file.layers.back().get();

				const unsigned short LAYER_FLAGS = readUInt16(in);
				const unsigned short LAYER_TYPE = readUInt16(in);

				layer->flags = LAYER_FLAGS;
				layer->type = LAYER_TYPE;

				const unsigned short LAYER_CHILD_LEVEL = readUInt16(in);
				skipBytes(in, 4);

				layer->blendMode = static_cast<BlendMode>(readUInt16(in));
				layer->opacity = readUInt32(in);
				layer->name = readString(in, readUInt16(in));

#ifdef IS_NODE
				// layer node object
				layer->object = newObject;
				layer->objChildren = newArray;
				obj_push(objLayers, layer->object);
				layer->object["name"] = n_str(layer->name);
				layer->object["type"] = n_num(layer->type);
				layer->object["flags"] = n_num(layer->flags);
				layer->object["opacity"] = n_num(layer->opacity);
				layer->object["blendMode"] = n_num((uint16_t)layer->blendMode);
				layer->object["children"] = layer->objChildren;
#endif

				if (LAYER_CHILD_LEVEL == 0)
				{
					// Top layer
				}
				else
				{
					layer->layerParent = layerIndex[LAYER_CHILD_LEVEL - 1];
					layer->layerParent->layerChildren.push_back(layer);
#ifdef IS_NODE
					layer->object["layerParent"] = layer->layerParent->object;
					obj_push(layer->layerParent->objChildren, layer->object);
#endif
				}

				layerIndex[LAYER_CHILD_LEVEL] = layer;
			}
			break;

			case CHUNK_CEL:
			{
				file.cels.push_back(std::make_unique<Cel>());
				Cel *cel = file.cels.back().get();

				const unsigned short LAYER_INDEX = readUInt16(in);
				if (frame->cels.size() <= LAYER_INDEX)
					frame->cels.resize(LAYER_INDEX + 1, nullptr);

				cel->x = readInt16(in);
				cel->y = readInt16(in);
				cel->opacity = readUInt8(in);
				cel->frame = frame;
				cel->layer = file.layers[LAYER_INDEX].get();

				const unsigned short CEL_TYPE = readUInt16(in);
				skipBytes(in, 7);

				switch (CEL_TYPE)
				{
				case CEL_RAW:
				{
					cel->w = readUInt16(in);
					cel->h = readUInt16(in);

					const unsigned int CEL_DATA_LENGTH = CHUNK_SIZE - 26;
					cel->pixels = std::shared_ptr<uint8_t>(readBytes(in, CEL_DATA_LENGTH));
				}
				break;

				case CEL_LINKED:
				{
					const unsigned short CEL_LINK = readUInt16(in);

					cel->pixels = file.frames[CEL_LINK]->cels[LAYER_INDEX]->pixels;
					cel->w = file.frames[CEL_LINK]->cels[LAYER_INDEX]->w;
					cel->h = file.frames[CEL_LINK]->cels[LAYER_INDEX]->h;
					cel->link = CEL_LINK;
				}
				break;

				case CEL_COMPRESSED:
				{
					cel->w = readUInt16(in);
					cel->h = readUInt16(in);

					const unsigned int CEL_DATA_LENGTH = CHUNK_SIZE - 26;
					std::unique_ptr<uint8_t[]> cpixels(readBytes(in, CEL_DATA_LENGTH));

					unsigned long int celDataLengthUncompressed = cel->w * cel->h * bytesPerPixel;
					cel->pixels = std::shared_ptr<uint8_t>(new uint8_t[celDataLengthUncompressed]);

					int ret = uncompress(cel->pixels.get(), &celDataLengthUncompressed, cpixels.get(), CEL_DATA_LENGTH);

					if (ret != Z_OK)
						throw ResourceLoadException("Data decompression failed");
				}
				break;

				default:
					throw ResourceLoadException("Invalid/unsupported Aseprite cel type");
					break;
				}

				frame->cels[LAYER_INDEX] = cel;

#ifdef IS_NODE
				cel->object = newObject;

				obj_push(objCels, cel->object);
				cel->object["x"] = n_num(cel->x);
				cel->object["y"] = n_num(cel->y);
				cel->object["w"] = n_num(cel->w);
				cel->object["h"] = n_num(cel->h);
				cel->object["opacity"] = n_num(cel->opacity);
				cel->object["frame"] = frame->object;
				cel->object["layer"] = cel->layer->object;

				if (CEL_TYPE == CEL_LINKED)
				{
					cel->objPixels = file.frames[cel->link]->cels[LAYER_INDEX]->objPixels;
				}
				else
				{
					int dataLength = cel->w * cel->h * (file.colorDepth / 8);
					cel->objPixels = Uint8Array::New(env, dataLength);
					uint8_t *pPixel = cel->pixels.get();
					for (int i = 0; i < dataLength; i++, pPixel++)
					{
						cel->objPixels[i] = *pPixel;
					}
				}
				cel->object["pixels"] = cel->objPixels;

				frame->objCels[LAYER_INDEX] = cel->object;
#endif
			}
			break;

			case CHUNK_CELEXTRA:
				skipBytes(in, CHUNK_SIZE - 4 - 2);
				break;

			case CHUNK_FRAME_TAGS:
			{
				const unsigned short TAG_COUNT = readUInt16(in);
				skipBytes(in, 8);

				for (unsigned idxTag = 0u; idxTag < TAG_COUNT; ++idxTag)
				{
					file.tags.push_back(std::make_unique<FrameTag>());
					FrameTag *tag = file.tags.back().get();

					tag->frameFrom = readUInt16(in);
					tag->frameTo = readUInt16(in);
					tag->direction = static_cast<AnimationDirection>(readUInt16(in));
					skipBytes(in, 7);

					const unsigned int TAG_COLOR = readUInt32(in);
					tag->color.r = TAG_COLOR & 0xff;
					tag->color.g = (TAG_COLOR >> 8) & 0xff;
					tag->color.b = (TAG_COLOR >> 16) & 0xff;
					tag->color.a = (TAG_COLOR >> 24) & 0xff;
					tag->name = readString(in, readUInt16(in));

#ifdef IS_NODE
					tag->object = newObject;
					tag->objFrames = newArray;
					obj_push(objTags, tag->object);
					tag->object["from"] = n_num(tag->frameFrom);
					tag->object["to"] = n_num(tag->frameTo);
					tag->object["direction"] = n_num((uint8_t)tag->direction);
					tag->object["name"] = n_str(tag->name);
					set_color(tag->object["color"], tag->color);
					tag->object["frames"] = tag->objFrames;
#endif
				}
			}
			break;

			case CHUNK_PALETTE:
			{
				const unsigned long COLOR_COUNT = readUInt32(in);

				file.palette->paletteSize = COLOR_COUNT;
				file.palette->firstColor = readUInt32(in);
				file.palette->lastColor = readUInt32(in);
				skipBytes(in, 8);

#ifdef IS_NODE
				file.palette->objColors = newArray;
				objPalette["size"] = n_num(COLOR_COUNT);
				objPalette["firstColor"] = n_num(file.palette->firstColor);
				objPalette["lastColor"] = n_num(file.palette->lastColor);
				objPalette["colors"] = file.palette->objColors;
#endif

				for (unsigned i = 0u; i < COLOR_COUNT; ++i)
				{
					file.palette->colors.push_back(std::make_unique<Color>());
					Color *color = file.palette->colors.back().get();
					bool hasName = readUInt16(in) & 1;

					const unsigned int COLOR = readUInt32(in);
					color->r = COLOR & 0xff;
					color->g = (COLOR >> 8) & 0xff;
					color->b = (COLOR >> 16) & 0xff;
					color->a = (COLOR >> 24) & 0xff;

					if (hasName)
						readString(in, readUInt16(in));

#ifdef IS_NODE
					Array objColor = Array::New(env, 4);
					objColor[0u] = n_num(color->r);
					objColor[1u] = n_num(color->g);
					objColor[2u] = n_num(color->b);
					objColor[3u] = n_num(color->a);
					obj_push(file.palette->objColors, objColor);
#endif
				}
			}
			break;

			case CHUNK_USERDATA:
				skipBytes(in, CHUNK_SIZE - 4 - 2);
				break;

			case CHUNK_SLICE:
			{
				file.slices.push_back(std::make_unique<Slice>());
				Slice *slice = file.slices.back().get();

				const unsigned int SLICE_KEYS = readUInt32(in);
				const unsigned int SLICE_FLAGS = readUInt32(in);
				skipBytes(in, 4);
				slice->has9Slice = SLICE_FLAGS & FLAG_SLICE_9SLICES;
				slice->hasPivot = SLICE_FLAGS & FLAG_SLICE_PIVOT;
				slice->name = readString(in, readUInt16(in));

#ifdef IS_NODE
				slice->object = newObject;
				slice->objKeys = newArray;
				obj_push(objSlices, slice->object);
				slice->object["name"] = n_str(slice->name);
				slice->object["has9Slice"] = Boolean::New(env, slice->has9Slice);
				slice->object["hasPivot"] = Boolean::New(env, slice->hasPivot);
				slice->object["keys"] = slice->objKeys;
#endif

				for (unsigned i = 0u; i < SLICE_KEYS; i++)
				{
					slice->keys.push_back(std::make_unique<SliceKey>());
					SliceKey *key = slice->keys.back().get();

					key->frame = readUInt32(in);
					key->x = readUInt32(in);
					key->y = readUInt32(in);
					key->w = readUInt32(in);
					key->h = readUInt32(in);

#ifdef IS_NODE
					Object objKey = newObject;
					objKey["frame"] = n_num(key->frame);
					objKey["x"] = n_num(key->x);
					objKey["y"] = n_num(key->y);
					objKey["w"] = n_num(key->w);
					objKey["h"] = n_num(key->h);
					obj_push(slice->objKeys, objKey);
#endif

					if (slice->has9Slice)
					{
						key->patchX = readUInt32(in);
						key->patchY = readUInt32(in);
						key->patchW = readUInt32(in);
						key->patchH = readUInt32(in);

#ifdef IS_NODE
						Object obj9Slice = newObject;
						obj9Slice["x"] = n_num(key->patchX);
						obj9Slice["y"] = n_num(key->patchY);
						obj9Slice["w"] = n_num(key->patchW);
						obj9Slice["h"] = n_num(key->patchH);
						objKey["patch"] = obj9Slice;
#endif
					}
					if (slice->hasPivot)
					{
						key->pivotX = readUInt32(in);
						key->pivotY = readUInt32(in);

#ifdef IS_NODE
						Object objPivot = newObject;
						objPivot["x"] = n_num(key->pivotX);
						objPivot["y"] = n_num(key->pivotY);
						objKey["pivot"] = objPivot;
#endif
					}
				}
			}
			break;

			default:
				skipBytes(in, CHUNK_SIZE - 4 - 2);
				break;
			}
		}

		frame->cels.resize(file.layers.size(), nullptr);
	}

	// Tag frames
	for (auto &tag : file.tags)
	{
		for (int i = tag->frameFrom; i <= tag->frameTo; ++i)
		{
			tag->frames.push_back(file.frames[i].get());
			file.frames[i]->tags.push_back(tag.get());

#ifdef IS_NODE
			obj_push(tag->objFrames, file.frames[i]->object);
			obj_push(file.frames[i]->objTags, tag->object);
#endif
		}
	}
}
