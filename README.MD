# node-aseprite-reader

Aseprite file parser for C++ and Node, using C++ addon.

Based on source code from [cremmy/AsepriteReader](https://github.com/cremmy/AsepriteReader)

## Build from source

- Configure node-gyp globally, follow the instructions here: https://github.com/nodejs/node-gyp
- `node-gyp rebuild`

## Quick example

Pass the buffer of the Aseprite file to the function, it will return parsed file information as object.

```js
const readAseprite = require('aseprite-reader');
const fs = require('fs');

const buffer = fs.readFileSync('./test.aseprite');
const ase = readAseprite(buffer);

for (const layer of ase.layers) {
	console.log(layer.name);
}
```

## JS Interfaces

### Default export: `function(buffer): Aseprite`

Parses the Aseprite file and returns the [Aseprite](#aseprite-object) object.

### `Aseprite` object

| Property     | Type                       | Description                  |
|--------------|----------------------------|------------------------------|
| `width`      | number                     | Width in pixels              |
| `height`     | number                     | Height in pixels             |
| `numFrames`  | number                     | Number of frames in the file |
| `colorDepth` | number                     | Color depth in bits. 32 = RGBA, 16 = Greyscale, 8 = Indexed |
| `numColors`  | number                     | Number of colors in palette  |
| `pixelRatio` | number                     | Pixel width:Pixel height     |
| `palette`    | [Palette](#palette-object) | Palette object               |
| `frames`     | [Frame](#frame-object)[]   | Array of Frame objects       |
| `tags`       | [Tag](#tag-object)[]       | Array of Tag objects         |
| `layers`     | [Layer](#layer-object)[]   | Array of Layer objects       |
| `cels`       | [Cel](#cel-object)[]       | Array of Cel objects         |
| `slices`     | [Slice](#slice-object)[]   | Array of Slice objects       |

### `Palette` object

| Property     | Type       | Description                       |
|--------------|------------|-----------------------------------|
| `size`       | number     | Number of colors in palette       |
| `firstColor` | number     | Index of the first color          |
| `lastColor`  | number     | Index of the last color           |
| `colors`     | number[][] | Array of colors in `[r, g, b, a]` |

### `Frame` object

| Property     | Type                 | Description                           |
|--------------|----------------------|---------------------------------------|
| `duration`   | number               | Duration (in ms)                      |
| `cels`       | ([Cel](#cel-object) \| undefined)[] | Array of cels on this frame, match the order of layers. <br>If the layer has no cel on this frame, the item is undefined. |
| `tags`       | [Tag](#tag-object)[] | Array of tags that include this frame |

### `Layer` object

| Property      | Type    | Description                                    |
|---------------|---------|------------------------------------------------|
| `name`        | string  | Layer name                                     |
| `index`       | number  | Index of the layer in Aseprite.layers array    |
| `type`        | number  | Layer type. 0 = Normal, 1 = Group, 2 = Tilemap |
| `flags`       | number  | Layer flags (see [spec](#more-info))           |
| `opacity`     | number  | Opacity (0-255)                                |
| `blendMode`   | number  | Layer blend mode (see [spec](#more-info))      |
| `children`    | Layer[] | Array of child layers for layer group          |
| `layerParent` | Layer?  | The parent layer (if this is a child layer)    |

### `Cel` object

| Property  | Type                   | Description                     |
|-----------|------------------------|---------------------------------|
| `x`       | number                 | X position in the sprite        |
| `y`       | number                 | Y position in the sprite        |
| `w`       | number                 | Width of the cel in the sprite  |
| `h`       | number                 | Height of the cel in the sprite |
| `opacity` | number                 | Opacity (0-255)                 |
| `frame`   | [Frame](#frame-object) | Frame object of the cel         |
| `layer`   | [Layer](#layer-object) | Layer object of the cel         |
| `pixels`  | Uint8Array             | Raw cel pixel data              |

### `Tag` object

| Property    | Type     | Description                 |
|-------------|----------|-----------------------------|
| `name`      | string   | Tag name                    |
| `from`      | number   | First frame index           |
| `to`        | number   | Last frame index            |
| `direction` | number   | Loop animation direction. 0 = Forward, 1 = Reverse, 2 = Ping-pong |
| `color`     | number[] | Tag color in `[r, g, b, a]` |
| `frames`    | [Frame](#frame-object)[] | Array of frames included in this tag              |

### `Slice` object

| Property    | Type     | Description            |
|-------------|----------|------------------------|
| `name`      | string   | Slice name             |
| `has9Slice` | boolean  | It's a 9-patches slice |
| `hasPivot`  | boolean  | Has pivot information  |
| `keys`      | [SliceKey](#slicekey-object)[] | Array of keys in the slice |

### `SliceKey` object

| Property | Type   | Description                         |
|----------|--------|-------------------------------------|
| `frame`  | number | Frame number this slice starts from |
| `x`      | number | Slice X origin coordinate           |
| `y`      | number | Slice Y origin coordinate           |
| `w`      | number | Slice width                         |
| `h`      | number | Slice height                        |
| `patch`  | Rect?  | 9-patches slice info in `{ x, y, w, h }` (if it's a 9-patches slice) |
| `pivot`  | Point? | Pivot information in `{ x, y }` (if it has pivot information)        |

## Use the source code in C++

To use the source code in C++, remove `#define IS_NODE` for disabling Node API codes.

The reader depends on **zlib**.

```cpp
#include <stdio.h>
#include "aseprite-reader.h"

int main(int argc, char **argv) {
	AsepriteReader reader;

	char *buffer = /* file data */;
	int size = /* file size */;

	reader.load((uint8_t *)buffer, (uint32_t)size);

	for (auto &layer : reader.file.layers) {
		printf("%s\n", layer->name.c_str());
	}
}
```

## More info

Aseprite file spec: [Spec](https://github.com/aseprite/aseprite/blob/main/docs/ase-file-specs.md)
