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
        "},
	{ "colorful",
	  "" },
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
