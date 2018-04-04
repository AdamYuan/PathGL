//
// Created by adamyuan on 4/1/18.
//

#ifndef PATHTRACERGL_APPLICATION_HPP
#define PATHTRACERGL_APPLICATION_HPP

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
	int samples_;
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
	void init_ray_shader();
	void init_quad_shader();
	void check_work_groups();
	void destroy_gl_objects();
	void compute();
	void render_quad();
public:
	explicit Application();
	~Application();
	void Run();
};


#endif //PATHTRACERGL_APPLICATION_HPP
