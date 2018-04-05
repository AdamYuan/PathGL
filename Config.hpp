//
// Created by adamyuan on 4/1/18.
//

#ifndef CONFIG_HPP
#define CONFIG_HPP

constexpr float CAM_SPEED = 0.1f, CAM_ANGLE = 5.0f;

#define UNIT_X 48
#define UNIT_Y 32
#define WIDTH__ 1440
#define HEIGHT__ 900
constexpr unsigned GROUP_X = WIDTH__ / UNIT_X;
constexpr unsigned GROUP_Y = HEIGHT__ / UNIT_Y;
constexpr unsigned WIDTH = GROUP_X * UNIT_X;
constexpr unsigned HEIGHT = GROUP_Y * UNIT_Y;
#undef WIDTH__
#undef HEIGHT__

constexpr float ASPECT_RATIO = (float)WIDTH / (float)HEIGHT;

#define RAY_SHADER "shaders/pathtracer_plane.glsl"
#define FRAG_SHADER "shaders/quad.frag"
#define VERT_SHADER "shaders/quad.vert"

#endif
