#include <optimizedcache.h>

namespace
{
}

namespace vm
{
void OptimizedCache::PageSwapIn_Implement( void *currentLevelPage, const void *nextLevelPage )
{
	memcpy( currentLevelPage, nextLevelPage, GetPageSize() );
}
void OptimizedCache::PageSwapOut_Implement( void *nextLevelPage, const void *currentLevel )
{
	memcpy( nextLevelPage, currentLevel, GetPageSize() );
}
void OptimizedCache::PageWrite_Implement( void *currentLevelPage, const void *userData )
{
	memcpy( currentLevelPage, userData, GetPageSize() );
}
}  // namespace vm