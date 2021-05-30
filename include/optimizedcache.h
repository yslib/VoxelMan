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