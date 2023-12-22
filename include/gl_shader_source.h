/*
 * Copyright (C) Photon Vision.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

// clang-format off

static constexpr const char *VERTEX_SOURCE =
        "#version 100\n"
        ""
        "attribute vec2 vertex;"
        "varying vec2 texcoord;"
        ""
        "void main(void) {"
        "   texcoord = 0.5 * (vertex + 1.0);"
        "   gl_Position = vec4(vertex, 0.0, 1.0);"
        "}";


static constexpr const char *NONE_FRAGMENT_SOURCE =
        "#version 100\n"
        "#extension GL_OES_EGL_image_external : require\n"
        ""
        "precision lowp float;"
        "precision lowp int;"
        ""
        "varying vec2 texcoord;"
        ""
        "uniform samplerExternalOES tex;"
        ""
        "void main(void) {"
        "    vec3 color = texture2D(tex, texcoord).rgb;"
        "    gl_FragColor = vec4(color.bgr, 0);"
        "}";


static constexpr const char *HSV_FRAGMENT_SOURCE =
        "#version 100\n"
        "#extension GL_OES_EGL_image_external : require\n"
        ""
        "precision lowp float;"
        "precision lowp int;"
        ""
        "varying vec2 texcoord;"
        ""
        "uniform vec3 lowerThresh;"
        "uniform vec3 upperThresh;"
        "uniform bool invertHue;"
        "uniform samplerExternalOES tex;"
        ""
        "vec3 rgb2hsv(const vec3 p) {"
        "  const vec4 H = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);"
        // Using ternary seems to be faster than using mix and step
        "  vec4 o = mix(vec4(p.bg, H.wz), vec4(p.gb, H.xy), step(p.b, p.g));"
        "  vec4 t = mix(vec4(o.xyw, p.r), vec4(p.r, o.yzx), step(o.x, p.r));"
        ""
        "  float O = t.x - min(t.w, t.y);"
        "  const float n = 1.0e-10;"
        "  return vec3(abs(t.z + (t.w - t.y) / (6.0 * O + n)), O / (t.x + n), "
        "t.x);"
        "}"
        ""
        "bool inRange(vec3 hsv) {"
        "  const float epsilon = 0.0001;"
        "  bvec3 botBool = greaterThanEqual(hsv, lowerThresh - epsilon);"
        "  bvec3 topBool = lessThanEqual(hsv, upperThresh + epsilon);"
        "  if (invertHue) {"
        "    return !(botBool.x && topBool.x) && all(botBool.yz) && all(topBool.yz);"
        "  } else {"
        "    return all(botBool) && all(topBool);"
        "  }"
        "}"
        ""
        "void main(void) {"
        "  vec3 col = texture2D(tex, texcoord).rgb;"
        "  gl_FragColor = vec4(col.bgr, int(inRange(rgb2hsv(col))));"
        "}";


static constexpr const char *GRAY_PASSTHROUGH_FRAGMENT_SOURCE =
        "#version 100\n"
        "#extension GL_OES_EGL_image_external : require\n"
        ""
        "precision lowp float;"
        "precision lowp int;"
        ""
        "varying vec2 texcoord;"
        ""
        "uniform samplerExternalOES tex;"
        ""
        "void main(void) {"
        // We get in (gray, gray, gray), I think. So just copy the R channel
        "    vec3 gray_gray_gray = texture2D(tex, texcoord).rgb;"
        "    gl_FragColor = vec4(gray_gray_gray.bgr, gray_gray_gray[0]);"
        "}";


static constexpr const char *GRAY_FRAGMENT_SOURCE =
        "#version 100\n"
        "#extension GL_OES_EGL_image_external : require\n"
        ""
        "precision lowp float;"
        "precision lowp int;"
        ""
        "varying vec2 texcoord;"
        ""
        "uniform samplerExternalOES tex;"
        ""
        "void main(void) {"
        "    vec3 gammaColor = texture2D(tex, texcoord).rgb;"
        "    vec3 color = pow(gammaColor, vec3(2.0));"
        "    float gray = dot(color, vec3(0.2126, 0.7152, 0.0722));"
        "    float gammaGray = sqrt(gray);"
        "    gl_FragColor = vec4(color.bgr, gammaGray);"
        "}";


static constexpr const char *TILING_FRAGMENT_SOURCE =
        "#version 100\n"
        ""
        "precision lowp float;"
        "precision lowp int;"
        ""
        "uniform sampler2D tex;"
        "varying vec2 texcoord;"
        "uniform vec2 resolution_in;"
        ""
        "void main(void) {"
        "  float max_so_far = 0.0;"
        "  float min_so_far = 1.0;"
        "  for (int i = 0; i < 4; i++) {"
        "    for(int j = 0; j < 4; j++) {"
        "      vec2 offset = vec2(float(i), float(j)) / resolution_in;"
        "      float cur = texture2D(tex, texcoord + offset).w;"
        "      max_so_far = max(max_so_far, cur);"
        "      min_so_far = min(min_so_far, cur);"
        "    }"
        "  }"
        "  gl_FragColor = vec4(max_so_far, min_so_far, 0.0, 0.0);"
        "}";


static constexpr const char *THRESHOLDING_FRAGMENT_SOURCE =
        "#version 100\n"
        ""
        "precision lowp float;"
        "precision lowp int;"
        ""
        "uniform sampler2D tex;"
        "uniform sampler2D tiles;"
        "varying vec2 texcoord;"
        "uniform vec2 tile_resolution;"
        ""
        "void main(void) {"
        "  float max_so_far = 0.0;"
        "  float min_so_far = 1.0;"
        "  for (int i = -1; i <= 1; i++) {"
        "    for(int j = -1; j <= 1; j++) {"
        "      vec2 offset = vec2(float(i), float(j)) / tile_resolution;"
        "      vec2 cur = texture2D(tiles, texcoord + offset).xy;"
        "      max_so_far = max(max_so_far, cur.x);"
        "      min_so_far = min(min_so_far, cur.y);"
        "    }"
        "  }"
        ""
        "  float gray = texture2D(tex, texcoord).w;"
        "  vec3 color = texture2D(tex, texcoord).rgb;"
        "  float output_ = 0.5;"
        "  if ((max_so_far - min_so_far) > (0.1)) {"
        "    float mean = min_so_far + (max_so_far - min_so_far) / 2.0;"
        "    output_ = step(mean, gray);"
        "  }"
        "  gl_FragColor = vec4(color.bgr, output_);"
        "}";

// clang-format on
