//
// Created by adamyuan on 4/1/18.
//

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <string>

#include "Camera.hpp"

struct Resources
{
	GLuint vao, vbo, quad_tex;
};
struct RayShader
{
	GLuint program, seed_tex;
	GLint unif_cam_origin, unif_cam_forward, unif_cam_up, unif_cam_right, unif_cam_width, unif_cam_height, unif_last_sample;
};
struct QuadShader
{
	GLuint program;
};
class Application
{
private:
	bool locked_;
	const unsigned kSamplesPerCalculation, kInvocationX, kInvocationY, kWidth, kHeight, kGroupX, kGroupY;
	unsigned samples_;
	Resources res_;
	RayShader ray_;
	QuadShader quad_;

	GLFWwindow *window_;
	Camera cam_;

	static std::string read_file(const char *filename);
	static void key_callback(GLFWwindow *window, int key, int, int action, int);

	void screenshot();
	void init_window();
	void init_buffers();
	void init_texture();
	void init_ray_shader(const std::string &scene_glsl);
	void init_quad_shader();
	void check_gl();
	void destroy_gl_objects();
	void compute();
	void render_quad();
	unsigned long get_sps();
public:
	explicit Application(unsigned samples_per_calculation, unsigned width, unsigned height, unsigned invocation_size_x,
							 unsigned invocation_size_y, float cam_fov, float cam_speed, float cam_angle,
							 const char *scene_filename);
	~Application();
	void Run();
};


#endif
