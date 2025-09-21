#include "Texture.h"

Texture::Texture()
{
	textureID = 0;
	width = 0;
	height = 0;
	bitDepth = 0;
	fileLocation = "";
}

Texture::Texture(const char* fileLoc)
{
	textureID = 0;
	width = 0;
	height = 0; 
	bitDepth = 0;
	fileLocation = fileLoc;
}

void Texture::LoadTexture()
{
	// Load image data
	unsigned char* texData = stbi_load(fileLocation, &width, &height, &bitDepth, 0);

	// Check it
	if (!texData)
	{
		printf("Failed to find: %s\n", fileLocation);
		return;
	}

	// Generating texture, applying ID to it
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // linear blends pixels - Nearest is more pixelated

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);
	glGenerateMipmap(GL_TEXTURE_2D);

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
	// frees up texData since it's not needed
	stbi_image_free(texData);
}

void Texture::UseTexture()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::ClearTexture()
{
	// delete texture
	glDeleteTextures(1, &textureID);
	// reset params 
	textureID = 0;
	width = 0;
	height = 0;
	bitDepth = 0;
	//fileLocation = "";
}

Texture::~Texture()
{
	ClearTexture();
}