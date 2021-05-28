#include <voxelman.h>
#include <unordered_map>

using namespace vm;

namespace
{
Bound3f bound( { 0, 0, 0 }, { 1, 1, 1 } );

Point3f CubeVertices[ 8 ];

Point3f CubeTexCoords[ 8 ];

unsigned int CubeVertexIndices[] = {
	0, 2, 1, 1, 2, 3,
	4, 5, 6, 5, 7, 6,
	0, 1, 4, 1, 5, 4,
	2, 6, 3, 3, 6, 7,
	0, 4, 2, 2, 4, 6,
	1, 3, 5, 3, 7, 5
};

/**
 * @brief Transfer function presets
 * 
 */
std::unordered_map<std::string, std::string> TransferFunctionPresets = {
	{ "grayscale",
	  "2 0.0 1.0\n \
        0.0 0 0 0 0 0 0 0 0\n \
        1.0 255 255 255 255 255 255 255 255\
        " },
	{ "nucleon",
	  "12 0.0 1.0 \n \
		0.14115532 153 0 255 0 153 0 255 0\n \
		0.15267822 129 90 255 11 129 90 255 11\n \
		0.17271487 145 78 221 11 145 78 221 11\n \
		0.18580651 172 58 165 0 172 58 165 0\n \
		0.41194314 255 0 0 0 255 0 0 0\n \
		0.43786961 255 0 0 13 255 0 0 13\n \
		0.47819972 255 0 0 13 255 0 0 13\n \
		0.50115383 156 99 0 0 156 99 0 0\n \
		0.78643686 4 255 0 0 4 255 0 0\n \
		0.84693199 73 255 1 82 73 255 1 82\n \
		0.94993031 8 92 7 83 8 92 8 83\n \
		0.98955363 54 254 51 0 54 254 51 0\
		" },
	{ "blue",
	  "" },
	{ "green",
	  "" }
};
}  // namespace

std::string GetPresetTransferFunctionText( const std::string &name )
{
	return TransferFunctionPresets[ name ];
}
