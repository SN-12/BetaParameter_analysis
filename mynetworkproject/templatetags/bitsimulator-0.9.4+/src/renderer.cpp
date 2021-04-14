/*
 * Copyright (C) 2017-2019 Dominique Dhoutaut, Thierry Arrabal, Eugen Dedu.
 *
 * This file is part of BitSimulator.
 *
 * BitSimulator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * BitSimulator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with BitSimulator.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <chrono>
#include <cmath>
using std::chrono::steady_clock;
using std::chrono::duration;

#define GL_GLEXT_PROTOTYPES
#define GL3_PROTOTYPES 1
#ifdef HAVE_GL_GL_H
# include <GL/gl.h>
#else
# include <OpenGL/gl3.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <ft2build.h>
#include FT_GLYPH_H
#include FT_FREETYPE_H

#include "renderer.h"


struct FontCharInfo {
  // ID handle of the glyph texture
  GLuint texture;
  // Size of glyph
  unsigned int xSize;
  unsigned int ySize;
  // Offset from baseline to left/top of glyph
  int xBearing;
  int yBearing;
  // Offset to advance to next glyph
  long int Advance;
};

class RenderImpl {
public:
  RenderImpl() : projMatrix() { }

  // sdl bits.
  int width;
  int height;
  float dpi;
  SDL_Window* window = nullptr;
  // TTF_Font* font = nullptr;
  SDL_GLContext context = nullptr;

  // opengl bits.
  GLuint staticVao = 0;
  GLuint staticVbo = 0;
  GLuint dynamicVao = 0;
  GLuint dynamicVbo = 0;
  GLuint program = 0;
  const GLuint inPosition = 1; // the positions are pre-determined and
  const GLuint inColor = 2; // will not change.
  Matrix projMatrix;

  std::array<float, 3> focus;
  std::array<float, 3> position;
  Quaternion rotation;

  // fps counter.
  steady_clock::time_point fpsTime;
  unsigned int fpsCount = 0;
  unsigned int lastFps = 0;

  // text rendering.
  std::vector<Text> texts;
  std::map<char, FontCharInfo> chars;
  GLuint fontVao = 0;
  GLuint fontVbo = 0;
  GLuint fontProgram = 0;
  float fontSize = 0.1875f;

  // points.
  std::vector<Point> staticPoints;
  bool loadedStaticPoints;
  std::vector<Point> dynPoints;
  bool loadedDynPoints;
};


Renderer::Renderer()
  : impl(new RenderImpl()) {
}

Renderer::~Renderer() {
  delete impl;
}


void Renderer::detectWindowSize() {
  SDL_DisplayMode dm;
  if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
    std::cout << "SDL_GetDesktopDisplayMode failed: " << SDL_GetError() << "\n";
    impl->width = 640;
    impl->height = 480;
  }
  else {
    impl->width = dm.w * 0.8;
    impl->height = dm.h * 0.8;
  }

  if (SDL_GetDisplayDPI(0, &impl->dpi, NULL, NULL) != 0) {
    std::cout << "SDL_GetDisplayDPI failed, defaulting to 92 dpi.\n";
    impl->dpi = 92;
  }

  impl->fontSize = impl->dpi / 600.0;
}

Matrix clearProjMatrix(int width, int height) {
  // this sets the screen up so that it is [width] wide and [height] high,
  // and things inbetween [width] depth are drawn.

  float left = -width;
  float right = width;
  float top = height;
  float bottom = -height;
  float near = -width;
  float far = width;

  Matrix proj;

  #define at(x,y) (x*4+y)
  proj[at(0,0)] = 2 / (right - left);
  proj[at(0,1)] = 0;
  proj[at(0,2)] = 0;
  proj[at(0,3)] = 0;

  proj[at(1,0)] = 0;
  proj[at(1,1)] = 2 / (top - bottom);
  proj[at(1,2)] = 0;
  proj[at(1,3)] = 0;

  proj[at(2,0)] = 0;
  proj[at(2,1)] = 0;
  proj[at(2,2)] = -2 / (far - near);
  proj[at(2,3)] = 0;

  proj[at(3,0)] = -(right + left) / (right - left);
  proj[at(3,1)] = -(top + bottom) / (top - bottom);
  proj[at(3,2)] = -(far + near) / (far - near);
  proj[at(3,3)] = 1;
  #undef at

  return proj;
}

static const char* vertexShader =
  "#version 130\n"
  "in vec3 inPosition;\n"
  "in vec4 inColor;\n"
  "out vec4 passColor;\n"
  "uniform mat4 projMatrix;\n"
  "void main() {\n"
  "    passColor = inColor;\n"
  "    gl_Position = projMatrix * vec4( inPosition, 1.0 );\n"
  "}\n";

static const char* fragmentShader =
  "#version 130\n"
  "in vec4 passColor;\n"
  "out vec4 outColor;\n"
  "void main() {\n"
  "    outColor = passColor;\n"
  "}\n";

GLuint createShader(GLenum shaderType, const char* source) {
  GLuint shader = glCreateShader(shaderType);

  int length = strlen(source);
  glShaderSource(shader, 1, (GLchar**)&source, &length);
  glCompileShader(shader);

  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    GLint maxLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

    std::cout << "Could not compile shader. Error code: " << status <<
      ", message: " << errorLog.data() << "\n";
    exit(EXIT_FAILURE);
  }

  return shader;
}

void Renderer::createShaders() {
  GLuint vs = createShader(GL_VERTEX_SHADER, vertexShader);
  GLuint fs = createShader(GL_FRAGMENT_SHADER, fragmentShader);

  impl->program = glCreateProgram();
  glAttachShader(impl->program, vs);
  glAttachShader(impl->program, fs);

  glBindAttribLocation(impl->program, impl->inPosition, "inPosition");
  glBindAttribLocation(impl->program, impl->inColor, "inColor");
  glLinkProgram(impl->program);
  glUseProgram(impl->program);

  std::cout << "created and linked shaders sucessfully." << "\n";
}


static const char* fontVertexShader =
  "#version 330 core\n"
  "layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>\n"
  "out vec2 TexCoords;\n"
  "uniform mat4 projMatrix;\n"
  "void main()\n"
  "{\n"
  "  gl_Position = projMatrix * vec4(vertex.xy, 0.0, 1.0);\n"
  "  TexCoords = vertex.zw;\n"
  "}\n";

static const char* fontFragmentShader =
  "#version 330 core\n"
  "in vec2 TexCoords;\n"
  "out vec4 color;\n"
  "uniform sampler2D text;\n"
  "void main()\n"
  "{\n"
  "  vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
  "  color = vec4(0.2, 0.2, 0.2, 1.0) * sampled;\n"
  "}\n";


void Renderer::createFont(std::vector<std::string> fonts) {
  FT_Library library;
  if (FT_Init_FreeType(&library)) {
    std::cout << "FT_Init_FreeType() failed." << "\n";
    return;
  }

  FT_Face face;
  if (FT_New_Face(library, fonts[0].c_str(), 0, &face)) {
    std::cout << "FT_New_Face failed." << "\n";
    return;
  }

  // render the font at 128 px size.
  FT_Set_Char_Size(face, 128 * 64, 128 * 64, 96, 96);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  for (int c = 0; c < 128; c++) {
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      std::cout << "FT_LoadChar(" << c << ") failed.\n";
      continue;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width,
      face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
      face->glyph->bitmap.buffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    FontCharInfo character = { texture,
      face->glyph->bitmap.width, face->glyph->bitmap.rows,
      face->glyph->bitmap_left, face->glyph->bitmap_top,
      face->glyph->advance.x
    };
    impl->chars[c] = character;
  }

  FT_Done_Face(face);
  FT_Done_FreeType(library);

  GLuint vs = createShader(GL_VERTEX_SHADER, fontVertexShader);
  GLuint fs = createShader(GL_FRAGMENT_SHADER, fontFragmentShader);

  impl->fontProgram = glCreateProgram();
  glAttachShader(impl->fontProgram, vs);
  glAttachShader(impl->fontProgram, fs);
  glLinkProgram(impl->fontProgram);
  glUseProgram(impl->fontProgram);

  glGenVertexArrays(1, &impl->fontVao);
  glGenBuffers(1, &impl->fontVbo);
  glBindVertexArray(impl->fontVao);
  glBindBuffer(GL_ARRAY_BUFFER, impl->fontVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4,
    nullptr, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void Renderer::createWindow(std::string title) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cout << "SDL_Init failed: " << SDL_GetError() << "\n";
    exit(EXIT_FAILURE);
  }

  // require opengl attributes from sdl.
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
    SDL_GL_CONTEXT_PROFILE_CORE);

  detectWindowSize();

  impl->window = SDL_CreateWindow(title.c_str(),
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    impl->width, impl->height,
    SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
  if (impl->window == nullptr) {
    std::cout << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
    SDL_Quit();
    exit(EXIT_FAILURE);
  }

  impl->context = SDL_GL_CreateContext(impl->window);
  if (impl->context == nullptr) {
    std::cout << "SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
    SDL_Quit();
    exit(EXIT_FAILURE);
  }

  createShaders();

  std::vector<std::string> fonts = {
    "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
    "/Library/Fonts/Arial.ttf",
    "font.ttf"
  };

  createFont(fonts);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  glClearColor(0.9, 0.9, 0.9, 1);
  glViewport(0, 0, impl->width, impl->height);

  // set up a vertex buffer object for static points.
  glGenVertexArrays(1, &impl->staticVao);
  glGenBuffers(1, &impl->staticVbo);
  glBindVertexArray(impl->staticVao);
  glBindBuffer(GL_ARRAY_BUFFER, impl->staticVbo);

  glEnableVertexAttribArray(impl->inPosition);
  glEnableVertexAttribArray(impl->inColor);
  glVertexAttribPointer(impl->inPosition, 3, GL_FLOAT, GL_FALSE,
    sizeof(Point), 0);
  glVertexAttribPointer(impl->inColor, 4, GL_FLOAT, GL_FALSE,
    sizeof(Point), (void*)(3 * sizeof(float)));

  // and one more for dynamic points.
  glGenVertexArrays(1, &impl->dynamicVao);
  glGenBuffers(1, &impl->dynamicVbo);
  glBindVertexArray(impl->dynamicVao);
  glBindBuffer(GL_ARRAY_BUFFER, impl->dynamicVbo);

  glEnableVertexAttribArray(impl->inPosition);
  glEnableVertexAttribArray(impl->inColor);
  glVertexAttribPointer(impl->inPosition, 3, GL_FLOAT, GL_FALSE,
    sizeof(Point), 0);
  glVertexAttribPointer(impl->inColor, 4, GL_FLOAT, GL_FALSE,
    sizeof(Point), (void*)(3 * sizeof(float)));

  glUseProgram(impl->fontProgram);
  Matrix m = clearProjMatrix(impl->width, impl->height);
  glUniformMatrix4fv(glGetUniformLocation(impl->fontProgram, "projMatrix"),
    1, GL_FALSE, m.data());

  impl->loadedDynPoints = false;
  impl->loadedStaticPoints = false;
  impl->fpsTime = steady_clock::now();
}




void Renderer::setProjMatrix(const Matrix& matrix) {
  impl->projMatrix = matrix;

  glUseProgram(impl->program);
  glUniformMatrix4fv(glGetUniformLocation(impl->program, "projMatrix"),
    1, GL_FALSE, impl->projMatrix.data());
}


void Renderer::closeWindow() {
  for (auto c : impl->chars) {
    glDeleteTextures(1, &c.second.texture);
  }

  SDL_GL_DeleteContext(impl->context);
  SDL_DestroyWindow(impl->window);
  SDL_Quit();
}


void Renderer::redrawFont(const Text& text) {
  glUseProgram(impl->fontProgram);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(impl->fontVao);

  int x = text.x;
  for (auto c = text.text.begin(); c != text.text.end(); c++) {
    const FontCharInfo& ch = impl->chars[*c];

    GLfloat xpos = x + ch.xBearing * impl->fontSize;
    GLfloat ypos = text.y - (ch.ySize - ch.yBearing) * impl->fontSize;

    GLfloat w = ch.xSize * impl->fontSize;
    GLfloat h = ch.ySize * impl->fontSize;
    // Update VBO for each character
    GLfloat vertices[6][4] = {
      { xpos,     ypos + h,   0.0, 0.0 },
      { xpos,     ypos,       0.0, 1.0 },
      { xpos + w, ypos,       1.0, 1.0 },

      { xpos,     ypos + h,   0.0, 0.0 },
      { xpos + w, ypos,       1.0, 1.0 },
      { xpos + w, ypos + h,   1.0, 0.0 }
    };

    // Render glyph texture over quad
    glBindTexture(GL_TEXTURE_2D, ch.texture);
    glBindBuffer(GL_ARRAY_BUFFER, impl->fontVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // Now advance cursors for next glyph (note that advance is number 
    // of 1/64 pixels)
    x += (ch.Advance / 64) * impl->fontSize;
  }
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}


void Renderer::reloadPoints() {
  if (!impl->loadedStaticPoints) {
    glBindVertexArray(impl->staticVao);
    glBindBuffer(GL_ARRAY_BUFFER, impl->staticVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Point) * impl->staticPoints.size(),
      impl->staticPoints.data(), GL_STATIC_DRAW);
    impl->loadedStaticPoints = true;
  }

  if (!impl->loadedDynPoints) {
    glBindVertexArray(impl->dynamicVao);
    glBindBuffer(GL_ARRAY_BUFFER, impl->dynamicVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Point) * impl->dynPoints.size(),
      impl->dynPoints.data(), GL_DYNAMIC_DRAW);
    impl->loadedDynPoints = true;
  }
}

void Renderer::redraw() {
  reloadPoints();

  // draw point data.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPointSize(128/4 * impl->fontSize);

  glUseProgram(impl->program);
  glBindVertexArray(impl->staticVao);
  glBindBuffer(GL_ARRAY_BUFFER, impl->staticVbo);
  glDrawArrays(GL_POINTS, 0, impl->staticPoints.size());

  glPointSize(128/2 * impl->fontSize);

  glBindVertexArray(impl->dynamicVao);
  glBindBuffer(GL_ARRAY_BUFFER, impl->dynamicVbo);
  glDrawArrays(GL_POINTS, 0, impl->dynPoints.size());

  // draw fonts.
  for(auto text : impl->texts) {
    redrawFont(text);
  }
  impl->texts.clear();

  // finish drawing.
  SDL_GL_SwapWindow(impl->window);

  // count frames per second.
  impl->fpsCount++;
  steady_clock::time_point fin = steady_clock::now();
  std::chrono::duration<double> diff = fin - impl->fpsTime;
  if (diff.count() > 1.0) {
    impl->lastFps = impl->fpsCount;
    impl->fpsCount = 0;
    impl->fpsTime = fin;
  }

  auto fpstext = std::to_string(impl->lastFps) + " fps";
  addText(-impl->width + 128/2 * impl->fontSize,
    -impl->height + 128 * impl->fontSize,
    fpstext);
}



void Renderer::addStaticPoint(const Point& point) {
  impl->staticPoints.push_back(point);
  impl->loadedStaticPoints = false;
}

void Renderer::clearDynamicPoints() {
  impl->dynPoints.clear();
  impl->loadedDynPoints = false;
}


void Renderer::addDynamicPoint(Point& point) {
  impl->dynPoints.push_back(point);
  impl->loadedDynPoints = false;
}


void Renderer::addDynamicPoint(float x, float y, float z,
  float r, float g, float b, float a) {
  Point p { x, y, z, r, g, b, a };
  addDynamicPoint(p);
}

void Renderer::addText(const Text& text) {
  impl->texts.push_back(text);
}

void Renderer::addText(float x, float y, const std::string& text) {
  impl->texts.push_back(Text {x, y, text});
}

std::pair<int,int> Renderer::getWindowSize() const {
  return std::make_pair(impl->width, impl->height);
}

Matrix naiveMatMul(const Matrix& a, const Matrix b) {
  Matrix h;

  #define at(x,y) (x*4+y)
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      h[at(r,c)] = 0;
      for (int j = 0; j < 4; j++) {
        h[at(r,c)] += a[at(j,c)] * b[at(r,j)];
      }
    }
  }
  #undef at

  return h;
}

Quaternion naiveQuatMul(const Quaternion& a, const Quaternion &b) {
  Quaternion q;
  q[0] = a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3];
  q[1] = a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2];
  q[2] = a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1];
  q[3] = a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0];
  return q;
}

Matrix getTranslationMatrix(const Vector& v) {
  // visually transposed, as in column-major style.
  return
    {    1,    0,    0, 0,
         0,    1,    0, 0,
         0,    0,    1, 0,
      v[0], v[1], v[2], 1 };
}


Matrix getRotationMatrix(const Quaternion& q) {
  float aa = q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3];
  float ba = 2 * (q[1] * q[2] - q[0] * q[3]);
  float ca = 2 * (q[0] * q[2] + q[1] * q[3]);
  float ab = 2 * (q[1] * q[2] + q[0] * q[3]);
  float bb = q[0] * q[0] - q[1] * q[1] + q[2] * q[2] - q[3] * q[3];
  float cb = 2 * (q[2] * q[3] - q[0] * q[1]);
  float ac = 2 * (q[1] * q[3] - q[0] * q[2]);
  float bc = 2 * (q[0] * q[1] + q[2] * q[3]);
  float cc = q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3];

  return
    { aa, ba, ca, 0,
      ab, bb, cb, 0,
      ac, bc, cc, 0,
       0,  0,  0, 1 };
}

Matrix getProjectionMatrix(float left, float right, float bottom,
  float top, float near, float far) {

  float aa = 2 * near / (right - left);
  float bb = 2 * near / (top - bottom);
  float ac = (right + left) / (right - left);
  float bc = (top + bottom) / (top - bottom);
  float cc = - (far - near) / (far - near);
  float ca = -2 * far * near / (far - near);

  return
    { aa,  0,  0,  0,
       0, bb,  0,  0,
      ac, bc, cc, -1,
       0,  0, ca,  0 };
}

void Renderer::resetView(float xoff, float yoff, float zoff,
  float width, float height) {
  // compute the required distance to see at least width and height.
  width *= 1.1;
  height *= 1.1;
  float fovy = 70;
  float fovyInRadians = fovy * 2 * 3.141592 / 360;
  float fovxInRadians = fovyInRadians / impl->height * impl->width;
  float hdist = (height / 2) / std::tan(fovyInRadians / 2);
  float wdist = (width / 2) / std::tan(fovxInRadians / 2);
  float dist = zoff + std::max(hdist, wdist);

  // logTrace("x: {}, y: {}, width: {}, height: {}, dist: {}", xoff, yoff,
  //   width, height, dist);

  impl->focus[0] = -xoff;
  impl->focus[1] = -yoff;
  impl->focus[2] = -zoff;

  impl->position[0] = 0;
  impl->position[1] = 0;
  impl->position[2] = -dist;

  // reset rotation to 0.
  impl->rotation[0] = 1;
  impl->rotation[1] = 0;
  impl->rotation[2] = 0;
  impl->rotation[3] = 0;
  recomputeProjMatrix();
}


void Renderer::recomputeProjMatrix() {
  auto width = impl->width * 1e-2;
  auto height = impl->height * 1e-2;
  float fovy = 70;
  auto fovyInRadians = fovy * 2 * 3.141592 / 360;
  float near = height / std::tan(fovyInRadians / 2);
  float far = near * 1e12;
  Matrix focus = getTranslationMatrix(impl->focus);
  Matrix mov = getTranslationMatrix(impl->position);
  Matrix rot = getRotationMatrix(impl->rotation);
  Matrix proj = getProjectionMatrix(-width, width, -height, height, near, far);
  Matrix fin =
    naiveMatMul(proj,
      naiveMatMul(mov,
        naiveMatMul(rot, focus)));
  setProjMatrix(fin);
}


void Renderer::translate(const Vector& v) {
  impl->position[0] += v[0];
  impl->position[1] += v[1];
  impl->position[2] += v[2];
  recomputeProjMatrix();
}

void Renderer::translate(float x, float y, float z) {
  translate(Vector {x, y, z});
}


void Renderer::rotate(float yaw, float pitch, float roll) {
  double radYaw = yaw * 2 * 3.141592 / 360;
  double radPitch = pitch * 2 * 3.141592 / 360;
  double radRoll = roll * 2 * 3.141592 / 360;

  double cy = std::cos(radYaw * 0.5);
  double sy = std::sin(radYaw * 0.5);
  double cp = std::cos(radPitch * 0.5);
  double sp = std::sin(radPitch * 0.5);
  double cr = std::cos(radRoll * 0.5);
  double sr = std::sin(radRoll * 0.5);

  Quaternion q;
  q[0] = cy * cp * cr + sy * sp * sr;
  q[1] = cy * cp * sr - sy * sp * cr;
  q[2] = sy * cp * sr + cy * sp * cr;
  q[3] = sy * cp * cr - cy * sp * sr;

  impl->rotation = naiveQuatMul(impl->rotation, q);
  recomputeProjMatrix();
}
