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

#ifndef RENDERER_H_
#define RENDERER_H_

#include <utility>
#include <string>



using Matrix = std::array<float, 16>;
using Quaternion = std::array<float, 4>;
using Vector = std::array<float, 3>;
using Color = std::array<float, 4>;

/**
 * Points in space with a size.
 */
struct Point {
public:
  float x, y, z;
  float r, g, b, a;
};

/**
 * Text rendered at a specific point on the screen.
 *
 * x and y for text are screen-local in the dimensions [2*width, 2*height],
 * with [-width,-height] in the lower left corner.
 */
struct Text {
public:
  float x, y;
  std::string text;
};

// opaque class containing opengl specific info.
// Not provided here so we don't have to include opengl headers.
class RenderImpl;

class Renderer {
private:
  RenderImpl* impl;

  void detectWindowSize();
  void createShaders();
  void createFont(std::vector<std::string> fonts);
  void redrawFont(const Text& text);
  void reloadPoints();
  void setProjMatrix(const Matrix& matrix);
  void recomputeProjMatrix();

public:
  Renderer();
  ~Renderer();

  void createWindow(std::string title);
  void redraw();
  void closeWindow();

  std::pair<int,int> getWindowSize() const;
  void resetView(float xoff, float yoff, float zoff, float width, float height);
  void translate(const Vector& v);
  void translate(float x, float y, float z);
  void rotate(float yaw, float pitch, float roll);

  void addStaticPoint(const Point& point);

  void clearDynamicPoints();
  void addDynamicPoint(Point& point);
  void addDynamicPoint(float x, float y, float z,
    float r, float g, float b, float a);

  void addText(const Text& text);
  void addText(float x, float y, const std::string& text);
};


#endif /* RENDERER_H_ */
