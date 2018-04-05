//
// Created by adamyuan on 4/1/18.
//

#ifndef CONFIG_HPP
#define CONFIG_HPP

constexpr int UNIT_X = 40;
constexpr int UNIT_Y = 20;
constexpr int WIDTH__ = 1080;
constexpr int HEIGHT__ = 720;

constexpr unsigned GROUP_X = WIDTH__ / UNIT_X;
constexpr unsigned GROUP_Y = HEIGHT__ / UNIT_Y;
constexpr unsigned WIDTH = GROUP_X * UNIT_X;
constexpr unsigned HEIGHT = GROUP_Y * UNIT_Y;
#define RAY_SHADER "shaders/pathtracer.glsl"
#define FRAG_SHADER "shaders/quad.frag"
#define VERT_SHADER "shaders/quad.vert"

#endif
