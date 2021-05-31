#pragma once
#include <VMFoundation/largevolumecache.h>
namespace vm
{
class MortonCodeCache final : public Block3DCache
{
public:
	MortonCodeCache( IRefCnt *cnt, I3DBlockDataInterface *pageFile, std::function<Size3( I3DBlockDataInterface * )> evaluator ) :
	  Block3DCache( cnt, pageFile, std::move( evaluator ) ) {}
	MortonCodeCache( IRefCnt *cnt, I3DBlockDataInterface *pageFile ) :
	  Block3DCache( cnt, pageFile ) {}

protected:
	void PageSwapIn_Implement( void *currentLevelPage, const void *nextLevelPage ) override final;
	void PageSwapOut_Implement( void *nextLevelPage, const void *currentLevel ) override final;
	void PageWrite_Implement( void *currentLevelPage, const void *userData ) override final;
};
}  // namespace vm

// extern const uint32_t morton256_x[256];
// extern const uint32_t morton256_y[256];
// extern const uint32_t morton256_z[256];

inline uint64_t MortonEncode_LUT( unsigned int x, unsigned int y, unsigned int z )
{
	uint64_t answer = 0;
	answer = morton256_z[ ( z >> 16 ) & 0xFF ] |  // we start by shifting the third byte, since we only look at the first 21 bits
			 morton256_y[ ( y >> 16 ) & 0xFF ] |
			 morton256_x[ ( x >> 16 ) & 0xFF ];
	answer = answer << 48 | morton256_z[ ( z >> 8 ) & 0xFF ] |	// shifting second byte
			 morton256_y[ ( y >> 8 ) & 0xFF ] |
			 morton256_x[ ( x >> 8 ) & 0xFF ];
	answer = answer << 24 |
			 morton256_z[ (z)&0xFF ] |	// first byte
			 morton256_y[ (y)&0xFF ] |
			 morton256_x[ (x)&0xFF ];
	return answer;
}
