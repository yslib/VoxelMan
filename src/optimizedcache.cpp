#include <optimizedcache.h>

namespace vm
{
void MortonCodeCache::PageSwapIn_Implement( void *currentLevelPage, const void *nextLevelPage )
{
	memcpy( currentLevelPage, nextLevelPage, GetPageSize() );
}
void MortonCodeCache::PageSwapOut_Implement( void *nextLevelPage, const void *currentLevel )
{
	memcpy( nextLevelPage, currentLevel, GetPageSize() );
}
void MortonCodeCache::PageWrite_Implement( void *currentLevelPage, const void *userData )
{
	memcpy( currentLevelPage, userData, GetPageSize() );
}
}  // namespace vm
