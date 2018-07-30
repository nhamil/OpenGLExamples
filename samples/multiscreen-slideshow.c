/* Copyright (c) 2014-2015 Scott Kuhl. All rights reserved.
 * License: This code is licensed under a 3-clause BSD license. See
 * the file named "LICENSE" for a full copy of the license.
 */

/** @file Demonstrates drawing a 3D triangle.
 *
 * @author Scott Kuhl
 */

#include "libkuhl.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define IMAGE_DEPTH (4) 

#define DEFAULT_SCREEN_WIDTH (1920) 
#define DEFAULT_SCREEN_HEIGHT (1080) 

typedef struct _vec2
{
	float x, y; 
} Vec2;

typedef struct _imageinfo
{
	kuhl_geometry quad; 
	float startTime; 
	float duration; 
	float fadeIn; 
	float fadeOut; 
	Vec2 position; 
	Vec2 size; 
	struct _imageinfo *next; 
} ImageInfo;

typedef struct _parser 
{
	char *text; 
	const char *end; 
	const char *pos; 
	int line; 
	int linePos; 
} Parser;

static GLuint textureShader = 0; 
static ImageInfo *imageInfo = NULL; 
static double time = 0.0; 

void InitVec2(Vec2 *self, float x, float y) 
{
	self->x = x; 
	self->y = y; 
}

static void ImageInfoGenerateQuad(ImageInfo *self, const char *filename) 
{
	kuhl_geometry_new(&self->quad, textureShader, 4, GL_TRIANGLES); 

	GLfloat positions[] = 
	{
		0, 0, 
		1, 0, 
		1, 1, 
		0, 1
	};
	kuhl_geometry_attrib(&self->quad, positions, 2, "a_Position", KG_WARN); 

	GLfloat texCoords[] = 
	{
		0, 0, 
		1, 0, 
		1, 1, 
		0, 1 
	};
	kuhl_geometry_attrib(&self->quad, texCoords, 2, "a_TexCoord", KG_WARN); 

	GLuint indices[] = 
	{
		0, 1, 2, 
		0, 2, 3 
	};
	kuhl_geometry_indices(&self->quad, indices, 6); 

	GLuint texId = 0; 
	kuhl_read_texture_file(filename, &texId); 
	kuhl_geometry_texture(&self->quad, texId, "u_Texture", KG_WARN); 

	kuhl_errorcheck();
}

void CreateImageInfo(ImageInfo *self, const char *filename, float x, float y, float w, float h) 
{
	ImageInfoGenerateQuad(self, filename); 

	self->startTime = 0.0f; 
	self->duration = 5.0f; 
	self->fadeIn = 1.0f; 
	self->fadeOut = 1.0f; 
	self->next = NULL; 

	InitVec2(&self->position, x, y); 
	InitVec2(&self->size, w, h); 
}

void DestroyImageInfo(ImageInfo *self) 
{
	for (unsigned i = 0; i < self->quad.texture_count; i++) 
	{
		// TODO is this needed? 
		glDeleteTextures(1, &self->quad.textures[i].textureId); 

		// TODO free [name]? 
		const char *name = self->quad.textures[i].name; 
		msg(MSG_INFO, "Deleted texture \"%s\"\n", name); 
	}

	kuhl_geometry_delete(&self->quad); 
}

ImageInfo *NewImageInfo(const char *filename, float x, float y, float w, float h) 
{
	ImageInfo *self = malloc(sizeof (ImageInfo)); 
	CreateImageInfo(self, filename, x, y, w, h); 
	return self; 
}

void FreeImageInfo(ImageInfo *self) 
{
	DestroyImageInfo(self); 
	free(self); 
}

void ImageInfoPreDraw(float ortho[4])  
{
	glUseProgram(textureShader);  
	glDisable(GL_DEPTH_TEST); 
	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

	float view[16];  
	mat4f_ortho_new(view, 
		ortho[0] * -1, 
		ortho[1] * 1, 
		ortho[2] * -1, 
		ortho[3] * 1, 
		1, -1
	); 
	glUniformMatrix4fv(kuhl_get_uniform("u_ViewMat"), 1, GL_FALSE, view); 
	glUniform1f(kuhl_get_uniform("u_Depth"), 0.0f); 
}

void ImageInfoPostDraw(void) 
{
	glEnable(GL_DEPTH_TEST); 
}

void ImageInfoDraw(ImageInfo *self) 
{
	float alpha = 0.0f; 

	if (time >= self->startTime) 
	{
		float curDuration = time - self->startTime; 

		if (curDuration < self->duration) 
		{
			if (self->fadeIn > 0 && curDuration < self->fadeIn) 
			{
				alpha = curDuration / self->fadeIn; 
			}
			else if (self->fadeOut > 0 && self->duration - curDuration < self->fadeOut) 
			{
				alpha = (self->duration - curDuration) / self->fadeOut; 
			}
			else 
			{
				alpha = 1.0f; 
			}
		}
	}

	if (alpha > 0.0f) 
	{
		glUniform1f(kuhl_get_uniform("u_Alpha"), alpha); 

		float model[16], tmp[16], tmp2[16]; 
		mat4f_translate_new(tmp, self->position.x * 2 - 1, self->position.y * 2 - 1, 0.0f); 
		mat4f_scale_new(tmp2, self->size.x * 2, self->size.y * 2, 1.0f); 
		mat4f_mult_mat4f_new(model, tmp, tmp2); 

		glUniformMatrix4fv(kuhl_get_uniform("u_ModelMat"), 1, GL_FALSE, model); 
		kuhl_geometry_draw(&self->quad); 
	}
}

/* Called by GLFW whenever a key is pressed. */
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(action != GLFW_PRESS)
		return;
	
	switch(key)
	{
		case GLFW_KEY_Q:
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GL_TRUE);
			break;
	}
}

static inline void PrintFloatArray(unsigned count, float *arr) 
{
	char buf[1024]; 
	char *p = buf; 
	p += sprintf(p, "["); 
	for (unsigned i = 0; i < count; i++) 
	{
		if (i != 0) p += sprintf(p, ", "); 
		p += sprintf(p, "%f", arr[i]); 
	}
	p += sprintf(p, "]\n"); 
	msg(MSG_INFO, "%s", buf); 
}

/** Draws the 3D scene. */
void display()
{
	time = glfwGetTime(); 
	dgr_setget("time", &time, sizeof (double)); 

	/* Render the scene once for each viewport. Frequently one
	 * viewport will fill the entire screen. However, this loop will
	 * run twice for HMDs (once for the left eye and once for the
	 * right). */
	viewmat_begin_frame();
	for(int viewportID=0; viewportID<viewmat_num_viewports(); viewportID++)
	{
		viewmat_begin_eye(viewportID);

		/* Where is the viewport that we are drawing onto and what is its size? */
		int viewport[4]; // x,y of lower left corner, width, height
		viewmat_get_viewport(viewport, viewportID);
		/* Tell OpenGL the area of the window that we will be drawing in. */
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

		/* Clear the current viewport. Without glScissor(), glClear()
		 * clears the entire screen. We could call glClear() before
		 * this viewport loop---but in order for all variations of
		 * this code to work (Oculus support, etc), we can only draw
		 * after viewmat_begin_eye(). */
		glScissor(viewport[0], viewport[1], viewport[2], viewport[3]);
		glEnable(GL_SCISSOR_TEST);
		glClearColor(0,0,0,0); // set clear color to grey
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);
		kuhl_errorcheck();

		float frustum[6], master[6], viewCoords[4]; 
		viewmat_get_frustum(frustum, viewportID); 
		viewmat_get_master_frustum(master); 

		for (unsigned i = 0; i < 4; i++) viewCoords[i] = frustum[i] / master[i]; 

		ImageInfoPreDraw(viewCoords); 
		ImageInfo *cur = imageInfo; 
		while (cur) 
		{
			ImageInfoDraw(cur); 
			cur = cur->next; 
		}
		ImageInfoPostDraw(); 

		glUseProgram(0); // stop using a GLSL program.
		viewmat_end_eye(viewportID);
	} // finish viewport loop
	viewmat_end_frame();

	/* Check for errors. If there are errors, consider adding more
	 * calls to kuhl_errorcheck() in your code. */
	kuhl_errorcheck();

}

int IsAlpha(char c) 
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); 
}

int IsNumber(char c) 
{
	return c >= '0' && c <= '9'; 
}

int StringStartsWith(const char *str, const char *with) 
{
	size_t withLen = strlen(with); 
	return strlen(str) < withLen ? 0 : strncmp(str, with, withLen) == 0; 
}

int StringStartsWithWord(const char *str, const char *with) 
{
	size_t strLen = strlen(str); 
	size_t withLen = strlen(with); 
	int startWith = strLen < withLen ? 0 : strncmp(str, with, withLen) == 0; 

	if (!startWith) return 0; 
	if (strLen == withLen) return 1; // can't have a char after if the max length is the same 

	char after = str[withLen]; 

	return !IsNumber(after) && !IsAlpha(after); 
}

void CreateParser(Parser *self, char *text) 
{
	self->text = text; 
	self->pos = text; 
	self->end = text + strlen(text); 
	self->line = 1; 
	self->linePos = 1; 
}

void DestroyParser(Parser *self) 
{
	free(self->text); 
}

int ParserAtEnd(Parser *self) 
{
	return self->pos >= self->end; 
}

char ParserPeek(Parser *self) 
{
	return (self->pos) < self->end ? *self->pos : 0; 
}

char ParserNext(Parser *self) 
{
	char c = ParserPeek(self); 
	
	if (self->pos > self->end) 
	{
		msg(MSG_FATAL, "At %d:%d, reached end of file\n", self->line, self->linePos); 
		exit(1); 
	}

	self->pos++; 

	if (c == '\n') 
	{
		self->line++; 
		self->linePos = 1; 
	}
	else 
	{
		self->linePos++; 
	}

	return c; 
}

void ParserNextN(Parser *self, unsigned count) 
{
	for (unsigned i = 0; i < count; i++) ParserNext(self); 
}

void ParserSkipWhitespace(Parser *self, int newlines) 
{
	while (!ParserAtEnd(self)) 
	{
		char c = ParserPeek(self); 

		if (c == ' ' || (newlines && c == '\n')) 
		{
			ParserNext(self); 
			continue; 
		}
		else 
		{
			return; 
		}
	}
}

void ParserExpect(Parser *self, const char *expect) 
{
	if (!StringStartsWith(self->pos, expect)) 
	{
		if (strcmp(expect, "\n") == 0) 
		{
			msg(MSG_FATAL, "At %d:%d, expected newline\n", self->line, self->linePos); 
			exit(1); 
		}
		else 
		{
			msg(MSG_FATAL, "At %d:%d, expected '%s'\n", self->line, self->linePos, expect); 
			exit(1); 
		}
	}

	ParserNextN(self, strlen(expect)); 
}

int ParserCheck(Parser *self, const char *check) 
{
	if (StringStartsWithWord(self->pos, check)) 
	{
		ParserNextN(self, strlen(check)); 
		return 1; 
	}
	else 
	{
		return 0; 
	}
}

float ParserGetFloat(Parser *self) 
{
	float value = 0; 
	int valid = 0; 

	while (!ParserAtEnd(self) && IsNumber(ParserPeek(self))) 
	{
		valid = 1; 
		value = value * 10 + (ParserNext(self) - '0'); 
	}

	if (!valid) 
	{
		msg(MSG_FATAL, "At %d:%d, expected number\n", self->line, self->linePos); 
		exit(1); 

		return value; 
	}

	if (ParserPeek(self) == '.') 
	{
		ParserNext(self); 

		float mul = 0.1f; 
		while (!ParserAtEnd(self) && IsNumber(ParserPeek(self))) 
		{
			value += (ParserNext(self) - '0') * mul;
			mul *= 0.1f;  
		}
	}

	return value; 
}

Vec2 ParserGetVec2(Parser *self) 
{
	Vec2 v; 

	v.x = ParserGetFloat(self); 
	ParserSkipWhitespace(self, 0); 
	ParserExpect(self, ","); 
	ParserSkipWhitespace(self, 0); 
	v.y = ParserGetFloat(self); 

	return v; 
}

void ParserGetString(Parser *self, char *out) 
{
	ParserExpect(self, "\""); 

	const char *start = self->pos; 

	while (!ParserAtEnd(self)) 
	{
		char c = ParserPeek(self); 

		if (c == '\"') 
		{
			size_t len = self->pos - start; 
			memcpy(out, start, len); 
			out[len] = 0; 
			ParserNext(self); 
			return; 
		}
		else 
		{
			ParserNext(self); 
		}
	}

	msg(MSG_FATAL, "At %d:%d, expected '\"'", self->line, self->linePos); 
	exit(1); 
}

float ParserAssignFloat(Parser *self) 
{
	float f;  

	ParserSkipWhitespace(self, 0); 
	ParserExpect(self, "="); 
	ParserSkipWhitespace(self, 0); 
	f = ParserGetFloat(self); 
	ParserSkipWhitespace(self, 0); 
	ParserExpect(self, "\n"); 

	return f; 
}

Vec2 ParserAssignVec2(Parser *self) 
{
	Vec2 v; 

	ParserSkipWhitespace(self, 0); 
	ParserExpect(self, "="); 
	ParserSkipWhitespace(self, 0); 
	v = ParserGetVec2(self); 
	ParserSkipWhitespace(self, 0); 
	ParserExpect(self, "\n"); 

	return v; 
}

ImageInfo *ParserGetImages(Parser *self) 
{
	Vec2 screen; 
	int screenSet = 0; 

	ImageInfo *cur = NULL; 

	while (!ParserAtEnd(self)) 
	{
		ParserSkipWhitespace(self, 1); 

		if (ParserCheck(self, "screen")) 
		{
			if (screenSet) 
			{
				msg(MSG_FATAL, "At %d:%d, screen dimensions have already been set\n", self->line, self->linePos); 
				exit(1); 
			}

			screenSet = 1; 
			screen = ParserAssignVec2(self); 

			msg(MSG_INFO, "Screen: %f, %f\n", screen.x, screen.y); 
		}
		else if (ParserCheck(self, "image")) 
		{
			if (!screenSet) 
			{
				msg(MSG_FATAL, "At %d:%d, screen dimensions have not been set, cannot define an image\n", self->line, self->linePos); 
				exit(1); 
			}

			ParserSkipWhitespace(self, 0); 
			char file[4096]; 
			ParserGetString(self, file); 
			ParserSkipWhitespace(self, 1); 
			ParserExpect(self, "{"); 

			Vec2 pos; 
			Vec2 size; 
			float start; 
			float duration; 
			float fadeIn; 
			float fadeOut; 

			while (!ParserCheck(self, "}")) 
			{
				ParserSkipWhitespace(self, 1); 

				if (ParserCheck(self, "position")) 
				{
					pos = ParserAssignVec2(self); 
				}
				else if (ParserCheck(self, "size")) 
				{
					size = ParserAssignVec2(self); 
				}
				else if (ParserCheck(self, "start")) 
				{
					start = ParserAssignFloat(self); 
				}
				else if (ParserCheck(self, "duration")) 
				{
					duration = ParserAssignFloat(self); 
				}
				else if (ParserCheck(self, "fadeIn")) 
				{
					fadeIn = ParserAssignFloat(self); 
				}
				else if (ParserCheck(self, "fadeOut")) 
				{
					fadeOut = ParserAssignFloat(self);  
				}
				else 
				{
					msg(MSG_FATAL, "At %d:%d, unexpected character\n", self->line, self->linePos); 
					exit(1); 
				}
			}

			ParserSkipWhitespace(self, 0); 
			ParserExpect(self, "\n"); 

			ImageInfo *info = NewImageInfo(file, pos.x / screen.x, pos.y / screen.y, size.x / screen.x, size.y / screen.y); 
			info->startTime = start; 
			info->duration = duration; 
			info->fadeIn = fadeIn; 
			info->fadeOut = fadeOut; 

			info->next = cur; 
			cur = info; 
		}
		else 
		{
			ParserNext(self); 
		}
	}

	return cur; 
}

int main(int argc, char **argv)
{
	/* Initialize GLFW and GLEW */
	kuhl_ogl_init(&argc, argv, 512, 512, 32, 4);

	/* Specify function to call when keys are pressed. */
	glfwSetKeyCallback(kuhl_get_window(), keyboard);
	// glfwSetFramebufferSizeCallback(window, reshape);

	const char *filename = "slideshow.txt"; 

	if (argc == 1) 
	{
		msg(MSG_INFO, "Running slideshow from default file 'slideshow.txt'\n"); 
	}
	else if (argc == 2) 
	{
		msg(MSG_INFO, "Running slideshow from file '%s'\n", argv[1]); 
		filename = argv[1]; 
	}
	else 
	{
		msg(MSG_FATAL, "Bad args: ./multiscreen-slideshow [slideshow file]\n"); 
		return -1; 
	}

	// setup texture shader 
	textureShader = kuhl_create_program("multiscreen-texture.vert", "multiscreen-texture.frag"); 
	glUseProgram(textureShader); 
	kuhl_errorcheck(); 

	char *slideshowConfig = kuhl_text_read(filename); 
	Parser parser; 
	CreateParser(&parser, slideshowConfig); 
	imageInfo = ParserGetImages(&parser); 
	DestroyParser(&parser); 

	/* Good practice: Unbind objects until we really need them. */
	glUseProgram(0);

	dgr_init();     /* Initialize DGR based on config file. */

	float initCamPos[3]  = {0,0,10}; // location of camera
	float initCamLook[3] = {0,0,0}; // a point the camera is facing at
	float initCamUp[3]   = {0,1,0}; // a vector indicating which direction is up
	viewmat_init(initCamPos, initCamLook, initCamUp);
	
	while(!glfwWindowShouldClose(kuhl_get_window()))
	{
		display();
		kuhl_errorcheck();

		/* process events (keyboard, mouse, etc) */
		glfwPollEvents();
	}

	exit(EXIT_SUCCESS);
}
