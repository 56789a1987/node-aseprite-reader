declare namespace AsepriteReader {
	export const enum TagDirection {
		Forward = 0,
		Reverse = 1,
		PingPong = 2,
	}

	export const enum LayerType {
		Normal = 0,
		Group = 1,
		Tilemap = 2,
	}

	export const enum LayerFlags {
		None = 0,
		Visible = 1,
		Editable = 2,
		LockMove = 4,
		Background = 8,
		PreferLinked = 16,
		GroupCollapsed = 32,
		Reference = 64,
	}

	export const enum BlendMode {
		Normal = 0,
		Multiply = 1,
		Screen = 2,
		Overlay = 3,
		Darken = 4,
		Lighten = 5,
		ColorDodge = 6,
		ColorBurn = 7,
		HardLight = 8,
		SoftLight = 9,
		Difference = 10,
		Exclusion = 11,
		Hue = 12,
		Saturation = 13,
		Color = 14,
		Luminosity = 15,
		Addition = 16,
		Subtract = 17,
		Divide = 18,
	}

	export type Color = [number, number, number, number];

	export interface Point {
		x: number;
		y: number;
	}

	export interface Rect {
		x: number;
		y: number;
		w: number;
		h: number;
	}

	export interface Aseprite {
		width: number;
		height: number;
		numFrames: number;
		colorDepth: number;
		numColors: number;
		pixelRatio: number;
		palette: Palette;
		frames: Frame[];
		tags: Tag[];
		layers: Layer[];
		cels: Cel[];
		slices: Slice[];
	}

	export interface Frame {
		duration: number;
		cels: (Cel | undefined)[];
		tags: Tag[];
	}

	export interface Layer {
		name: string;
		type: LayerType;
		flags: LayerFlags;
		opacity: number;
		blendMode: BlendMode;
		children: Layer[];
		layerParent?: Layer;
	}

	export interface Cel {
		x: number;
		y: number;
		w: number;
		h: number;
		opacity: number;
		frame: Frame;
		layer: Layer;
		pixels: Uint8Array;
	}

	export interface Tag {
		name: string;
		from: number;
		to: number;
		direction: TagDirection;
		color: Color;
		frames: Frame[];
	}

	export interface Palette {
		size: number;
		firstColor: number;
		lastColor: number;
		colors: Color[];
	}

	export interface Slice {
		name: string;
		has9Slice: boolean;
		hasPivot: boolean;
		keys: SliceKey[];
	}

	export interface SliceKey {
		frame: number;
		x: number;
		y: number;
		w: number;
		h: number;
		patch?: Rect;
		pivot?: Point;
	}
}

declare function AsepriteReader(buffer: Uint8Array): AsepriteReader.Aseprite;

export as namespace AsepriteReader;
export = AsepriteReader;
