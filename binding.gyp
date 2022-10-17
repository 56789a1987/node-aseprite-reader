{
	"targets": [
		{
			"target_name": "aseprite-reader",
			"cflags!": [ "-fno-exceptions" ],
			"cflags_cc!": [ "-fno-exceptions" ],
			"sources": [
				"./src/aseprite-reader.cpp",
				"./src/index.cpp"
			],
			"include_dirs": [
				"./src",
				"<!@(node -p \"require('node-addon-api').include\")"
			],
			'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ]
		}
	]
}
