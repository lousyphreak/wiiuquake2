#include "header/local.h"
#include <malloc.h>

#define ATTRIBUTE_BUFFER_ALIGNMENT (64)
#define ATTRIBUTE_BUFFER_SIZE (1024 * 1024 * 8)
static uint8_t *attributeBuffer;
static uint32_t attributeBufferOffset;

#define INDEX_BUFFER_ALIGNMENT (64)
#define INDEX_BUFFER_SIZE (1024 * 1024 * 8)
static uint8_t *indexBuffer;
static uint32_t indexBufferOffset;

#define UNIFORM_BUFFER_ALIGNMENT 0x100
#define UNIFORM_BUFFER_SIZE (1024 * 1024 * 1)
static uint8_t* uniformBuffer;
static uint32_t uniformBufferOffset;


void WiiU_InitDynBuffers()
{
	if(attributeBuffer != NULL)
		return;
   	attributeBuffer = memalign(ATTRIBUTE_BUFFER_ALIGNMENT, ATTRIBUTE_BUFFER_SIZE);
	attributeBufferOffset = 0;

   	indexBuffer = memalign(INDEX_BUFFER_ALIGNMENT, INDEX_BUFFER_SIZE);
	indexBufferOffset = 0;

   	uniformBuffer = memalign(UNIFORM_BUFFER_ALIGNMENT, UNIFORM_BUFFER_SIZE);
	uniformBufferOffset = 0;
}

void WiiU_ShutdownDynBuffers()
{
	if(attributeBuffer != NULL)
	{
		free(attributeBuffer);
		attributeBuffer = NULL;
	}
	attributeBufferOffset = 0;

	if(indexBuffer != NULL)
	{
		free(indexBuffer);
		indexBuffer = NULL;
	}
	indexBufferOffset = 0;

	if(uniformBuffer != NULL)
	{
		free(uniformBuffer);
		uniformBuffer = NULL;
	}
	uniformBufferOffset = 0;
}

void* WiiU_ABAlloc(uint32_t size)
{
	size += ATTRIBUTE_BUFFER_ALIGNMENT - 1;
	size &= ~(ATTRIBUTE_BUFFER_ALIGNMENT - 1);

	if(attributeBufferOffset + size > ATTRIBUTE_BUFFER_SIZE)
	{
		attributeBufferOffset = 0;
	}

	void *b = &attributeBuffer[attributeBufferOffset];
	DCZeroRange(b, size);
	attributeBufferOffset += size;

	return b;
}

void* WiiU_IBAlloc(uint32_t size)
{
	size += INDEX_BUFFER_ALIGNMENT - 1;
	size &= ~(INDEX_BUFFER_ALIGNMENT - 1);

	if(indexBufferOffset + size > INDEX_BUFFER_SIZE)
	{
		indexBufferOffset = 0;
	}

	void *b = &indexBuffer[indexBufferOffset];
	DCZeroRange(b, size);
	indexBufferOffset += size;

	return b;
}

void* WiiU_UBAlloc(uint32_t size)
{
	size += UNIFORM_BUFFER_ALIGNMENT - 1;
	size &= ~(UNIFORM_BUFFER_ALIGNMENT - 1);

	if(uniformBufferOffset + size > UNIFORM_BUFFER_SIZE)
	{
		uniformBufferOffset = 0;
	}

	void *b = &uniformBuffer[uniformBufferOffset];
	DCZeroRange(b, size);
	uniformBufferOffset += size;

	return b;
}
