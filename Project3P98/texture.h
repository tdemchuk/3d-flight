#ifndef CS3P98_TEXTURE_H
#define CS3P98_TEXTURE_H

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "aliases.h"
#include <glad/glad.h>		// OpenGL function pointers
#include <iostream>
#include <string>

/*
	COSC 3P98 - Term Project
	@author Tennyson Demchuk | 6190532 | td16qg@brocku.ca
	@author Daniel Sokic | 6164545 | ds16sz@brocku.ca
	@author Aditya Rajyaguru | 6582282 | ar18xp@brocku.ca
	@date 05.03.2021
*/
/*
	Loads a model into a GL texture
*/
class Texture {
private:


public:

	int width, height, channels;
	uint id;

	void load(const std::string& texname) {
		unsigned char* img = stbi_load(texname.c_str(), &width, &height, &channels, 0);
		if (!img) {
			printf("FATAL: Texture \"%s\" Load Failed.\n", texname.c_str());
			stbi_image_free(img);
			exit(0);
		}

		int imgmode = (channels == 3 ? GL_RGB : GL_RGBA);	// GL_RGBA if alpha channel present, GL_RBG if not

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, imgmode, GL_UNSIGNED_BYTE, img);
		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(img);
	}

	~Texture() {
		glDeleteTextures(1, &id);
	}
};

#endif