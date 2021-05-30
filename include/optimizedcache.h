#pragma once
#include <VMFoundation/largevolumecache.h>
namespace vm{
    class OptimizedCache:public Block3DCache{
        protected:
	 void PageSwapIn_Implement(void * currentLevelPage,const void * nextLevelPage)override;
	 void PageSwapOut_Implement(void * nextLevelPage, const void * currentLevel)override;
	 void PageWrite_Implement(void * currentLevelPage, const void * userData)override;
    };
}