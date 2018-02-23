////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_cache.c
//  Description    : This is the implementation of the cache for the CART
//                   driver.
//
//  Author         : [Jacob Hohenstein]
//  Last Modified  : [10/16/2016]
//

// Includes
#include <string.h> 
#include <stdlib.h>

// Project includes
#include <cart_cache.h>
#include <cart_driver.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>

// Defines

////////////////////////////////////////////////////////////////////////////////
//
// LRU Cache Structure
//
struct LRUCache_Frame
{
	CartridgeIndex cart_index;
	CartFrameIndex frame_index;
	char FrameData[CART_FRAME_SIZE];
	int access_time;
	int free;
};

struct LRUCache_Frame *LRUCache; 

uint32_t Cache_Max_Frames; //holds the maxium amount of frames in Cache
uint32_t Cache_Current_Time;
int Cache_Size = sizeof(struct LRUCache_Frame);

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : set_cart_cache_size
// Description  : Set the size of the cache (must be called before init)
//
// Inputs       : max_frames - the maximum number of items your cache can hold
// Outputs      : 0 if successful, -1 if failure

int set_cart_cache_size(uint32_t max_frames) {
	Cache_Max_Frames = max_frames;
	if(max_frames != Cache_Max_Frames)
		return (-1);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : init_cart_cache
// Description  : Initialize the cache and note maximum frames
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int init_cart_cache(void) {
	if(Cache_Max_Frames == 0) {
		Cache_Max_Frames= 15;
	}
	LRUCache = calloc(Cache_Max_Frames, Cache_Size;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : close_cart_cache
// Description  : Clear all of the contents of LRUCache_Frame,0,Cache_Max_Frames*DEFAULT_CART_FRAME_CACHE_SIZE);
	
	free(LRUCache);
	LRUCache = {0};
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : put_cart_cache
// Description  : Put an object into the frame 
//
// Inputs       : cart - the cartridge number of the frame to cache
//                frm - the frame number of the frame to cache
//                buf - the buffer to insert into the cache
// Outputs      : 0 if successful, -1 if failure

int put_cart_cache(CartridgeIndex cart, CartFrameIndex frm, void *buf)  {
	//Is object already in the cache?
	for(int i=0; i <Cache_Max_Frames;i++) {
		if(LRUCache[i].frame_index == frm && LRUCache[i].cart_index == cart && LRUCache[i].free ==1){
			//update access time
			LRUCache[i].access_time = Cache_Current_Time;
			//copy to cache
			memcpy(LRUCache[i].FrameData, buf, CART_FRAME_SIZE);
			Cache_Current_Time++;
			return 0;
		}
	}
	int index = -1;
	int Time_Min = 0; //keeps track of the index with the smallest lifespan

	//search cache for available slots
	for(int i=0; i< Cache_Max_Frames;i++){
		if(LRUCache[i].free ==0){
			index =i;
			break;
		}
		else if(LRUCache[i].access_time < LRUCache[Time_Min].access_time){
			Time_Min = i;
		}
	}
	//no avaliable slot found, begin LRU algorithm
	if(index == -1){
		index= Time_MIn;
	}
	//put object into the cache
	memcpy(LRUCache[index].FrameData, buf, CART_FRAME_SIZE);
	LRCache[index].cart_index= cart;
	LRUCache[index].free= 1;
	LRUCache[index].frame_index = frm;
	LRUCache[index].access_time = Cache_Current_Time;

	Cache_Current_Time++;
	return(0);
	
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : get_cart_cache
// Description  : Get an frame from the cache (and return it)
//
// Inputs       : cart_index - the cartridge number of the cartridge to find
//                frame_index - the  number of the frame to find
// Outputs      : pointer to cached frame or NULL if not found

void * get_cart_cache(CartridgeIndex cart_index, CartFrameIndex frame_index) {
	for(i=0;i<Cache_Max_Frames;i++)
	{
		if(LRUCache[i].cart_index==cart_index&&LRUCache[i].frame_index==frame_index &&LRUCache[i].free==1){
			LRUCache[i].access_time = Cache_Current_Time;
			Cache_Current_Time++;
			return LRUCache[i].FrameData;
		}
	}
	return NULL;	
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : delete_cart_cache
// Description  : Remove a frame from the cache (and return it)
//
// Inputs       : cart - the cart number of the frame to remove from cache
//                blk - the frame number of the frame to remove from cache
// Outputs      : pointe buffer inserted into the object

void * delete_cart_cache(CartridgeIndex cart, CartFrameIndex blk) {
	for(i=0;i<Cache_Max_Frames;i++)
	{
		if(LRUCache[i].cart_index==cart &&LRUCache[i].frame_index== blk &&LRUCache[i].free==1){
			LRUCache[i].free= 0;
			return LRUCache[i].FrameData;
		}
	}
	return NULL;
}

//
// Unit test

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cartCacheUnitTest
// Description  : Run a UNIT test checking the cache implementation
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int cartCacheUnitTest(void) {

	// Return successfully
	logMessage(LOG_OUTPUT_LEVEL, "Cache unit test completed successfully.");
	return(0);
}
