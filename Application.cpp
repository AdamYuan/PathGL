//
// Created by adamyuan on 4/1/18.
//

#include "Application.hpp"
#include "Config.hpp"
#include "SceneLoader.hpp"
#include <fstream>
#include <random>
#include <stb_image_write.h>

std::string Application::read_file(const char *filename)
{
	std::ifstream in(filename);
	return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void Application::check_gl()
{
	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("renderer: %s\n", glGetString(GL_RENDERER));
	printf("vendor: %s\n", glGetString(GL_VENDOR));

	int work_grp_cnt[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
	printf("max global (total) work group size x:%i y:%i z:%i\n",
		   work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

	int work_grp_size[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
	printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n",
		   work_grp_size[0], work_grp_size[1], work_grp_size[2]);

	int work_grp_inv;
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	printf("max local work group invocations %i\n", work_grp_inv);

	printf("\ncurrent size x:%u y:%u\ncurrent group count x:%u y:%u\ncurrent window size w:%u h:%u\n",
		   kInvocationX, kInvocationY, kGroupX, kGroupY, kWidth, kHeight);
}

void Application::init_window()
{
	gl3wInit();

	glfwInit();
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	window_ = glfwCreateWindow(kWidth, kHeight, "", nullptr, nullptr);
	glfwMakeContextCurrent(window_);

	glfwSetWindowUserPointer(window_, (void*)this);

	glfwSetKeyCallback(window_, key_callback);
}

void Application::init_buffers()
{
	constexpr GLfloat quad_vertices[] { -1.0f, -1.0f, 0.0f, 0.0f,
										1.0f, -1.0f, 1.0f, 0.0f,
										1.0f, 1.0f, 1.0f, 1.0f,
										1.0f, 1.0f, 1.0f, 1.0f,
										-1.0f, 1.0f, 0.0f, 1.0f,
										-1.0f, -1.0f, 0.0f, 0.0f };
	glGenBuffers(1, &res_.vbo);
	glGenVertexArrays(1, &res_.vao);

	glBindVertexArray(res_.vao);
	glBindBuffer(GL_ARRAY_BUFFER, res_.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GL_FLOAT), nullptr);//position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GL_FLOAT), (GLvoid*)(2 * sizeof(GL_FLOAT)));//texcoord
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

void Application::init_texture()
{
	glGenTextures(1, &res_.quad_tex);
	glBindTexture(GL_TEXTURE_2D, res_.quad_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, kWidth, kHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
	glBindImageTexture(0, res_.quad_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	std::mt19937 generator;
	auto *initial_seed = new GLuint[kWidth * kHeight * 4];
	for(int i = 0; i < kWidth * kHeight * 4; ++i)
		initial_seed[i] = (GLuint)(generator());
	glGenTextures(1, &ray_.seed_tex);
	glBindTexture(GL_TEXTURE_2D, ray_.seed_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, kWidth, kHeight, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, initial_seed);
	glBindImageTexture(1, ray_.seed_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32UI);

	delete[] initial_seed;
}

void Application::init_ray_shader(const std::string &scene_glsl)
{
	std::string ray_shader_str {read_file(RAY_SHADER)};
	ray_shader_str = scene_glsl + ray_shader_str;
	const char *ray_shader_src1 = ray_shader_str.c_str();
	char ray_shader_src[65536];
	sprintf(ray_shader_src,
			"#version 430 core\n"
			"layout(local_size_x = %u, local_size_y = %u) in;\n"
			"#define SAMPLES %u\n"
            "%s", kInvocationX, kInvocationY, kSamplesPerCalculation, ray_shader_src1);

	char *real_src = ray_shader_src;

	GLuint ray_shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(ray_shader, 1, &real_src, nullptr);
	glCompileShader(ray_shader);

	int success; glGetShaderiv(ray_shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		printf("%s\n", ray_shader_src);
		char info_log[65536]; glGetShaderInfoLog(ray_shader, 10000, nullptr, info_log);
		printf("%s\n\n\n", info_log);
	}

	ray_.program = glCreateProgram();
	glAttachShader(ray_.program, ray_shader);
	glLinkProgram(ray_.program);

	glDeleteShader(ray_shader);

	ray_.unif_cam_origin = glGetUniformLocation(ray_.program, "cam_origin");
	ray_.unif_cam_forward = glGetUniformLocation(ray_.program, "cam_forward");
	ray_.unif_cam_up = glGetUniformLocation(ray_.program, "cam_up");
	ray_.unif_cam_right = glGetUniformLocation(ray_.program, "cam_right");
	ray_.unif_cam_width = glGetUniformLocation(ray_.program, "cam_width");
	ray_.unif_cam_height = glGetUniformLocation(ray_.program, "cam_height");
	ray_.unif_last_sample = glGetUniformLocation(ray_.program, "last_sample");
}

void Application::init_quad_shader()
{
	std::string vert_shader_str = read_file(VERT_SHADER);
	std::string frag_shader_str = read_file(FRAG_SHADER);
	const char *vert_shader_src = vert_shader_str.c_str(), *frag_shader_src = frag_shader_str.c_str();
	GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER), frag_shader = glCreateShader(GL_FRAGMENT_SHADER);

	{
		glShaderSource(vert_shader, 1, &vert_shader_src, nullptr);
		glCompileShader(vert_shader);

		glShaderSource(frag_shader, 1, &frag_shader_src, nullptr);
		glCompileShader(frag_shader);
	}

	//link shaders
	{
		quad_.program = glCreateProgram();
		glAttachShader(quad_.program, vert_shader);
		glAttachShader(quad_.program, frag_shader);
		glLinkProgram(quad_.program);
	}

	glDeleteShader(vert_shader); glDeleteShader(frag_shader);
}

void Application::destroy_gl_objects()
{
	glDeleteVertexArrays(1, &res_.vao);
	glDeleteBuffers(1, &res_.vbo);
	glDeleteTextures(1, &res_.quad_tex);
	glDeleteTextures(1, &ray_.seed_tex);

	glDeleteProgram(ray_.program);
	glDeleteProgram(quad_.program);
}

void Application::render_quad()
{
	glUseProgram(quad_.program);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, res_.quad_tex);

	glBindVertexArray(res_.vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Application::compute()
{
	glUseProgram(ray_.program);
	cam_.SetUniform(ray_);
	glUniform1ui(ray_.unif_last_sample, samples_);
	glDispatchCompute(kGroupX, kGroupY, 1);
	// make sure writing to image has finished before read
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	samples_ += kSamplesPerCalculation;
}

Application::Application(unsigned samples_per_calculation, unsigned width, unsigned height, unsigned invocation_size_x,
						 unsigned invocation_size_y, float cam_fov, float cam_speed, float cam_angle,
						 const char *scene_filename) //in degrees
		: kSamplesPerCalculation(samples_per_calculation),
		  kInvocationX(invocation_size_x), kInvocationY(invocation_size_y),
		  kGroupX(width / invocation_size_x), kGroupY(height / invocation_size_y),
		  kWidth(width - (width % invocation_size_x)), kHeight(height - (height % invocation_size_y))
{
	samples_ = 0;
	locked_ = false;
	init_window();
	init_buffers();
	init_texture();
	init_quad_shader();
	try
	{
		SceneLoader loader{scene_filename};
		loader.SetCamera(&cam_);
		init_ray_shader(loader.GetGlsl());
	}
	catch (std::exception &e)
	{
		printf("scene parse error: %s\n", e.what());
	}
	check_gl();

	cam_.SetFov(cam_fov * 0.0174532925f * 0.5f);
	cam_.SetAngle(cam_angle);
	cam_.SetSpeed(cam_speed);
	cam_.SetAspectRatio((float)kWidth / (float)kHeight);
	cam_.Update();
}

Application::~Application()
{
	destroy_gl_objects();
}

void Application::Run()
{
	char title[512];
	while(!glfwWindowShouldClose(window_))
	{
		glfwPollEvents();

		glClear(GL_COLOR_BUFFER_BIT);
		cam_.Update();
		//don't change the order
		compute();
		render_quad();

		glfwSwapBuffers(window_);

		sprintf(title, "%u s/p %luk sp/s %s", samples_, get_sps() / 1000lu, locked_ ? " [locked]" : "");
		glfwSetWindowTitle(window_, title);
	}
}

void Application::screenshot()
{
	auto *pixels = new uint8_t[kWidth * kHeight * 3];
	auto *final = new uint8_t[kWidth * kHeight * 3];

	glReadPixels(0, 0, kWidth, kHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	for(int i = 0; i < kHeight; i ++)
		std::copy(pixels + i * kWidth * 3, pixels + (i + 1) * kWidth * 3, final + (kHeight - 1 - i) * kWidth * 3);

	char filename[256];
	sprintf(filename, "%uspp.png", samples_);
	stbi_write_png(filename, kWidth, kHeight, 3, final, 0);

	delete[] pixels;
	delete[] final;
}


void Application::key_callback(GLFWwindow *window, int key, int, int action, int)
{
	auto *app = (Application*)glfwGetWindowUserPointer(window);
	if(action != GLFW_RELEASE && !app->locked_ && app->cam_.Control(key))
		app->samples_ = 0;
	if(action == GLFW_PRESS)
	{
		if(key == GLFW_KEY_I)
			app->screenshot();
		else if(key == GLFW_KEY_L)
			app->locked_ = !app->locked_;
	}
}

unsigned long Application::get_sps()
{
	static unsigned last_time = 0;
	static unsigned long result = 0, sum = 0;

	sum += kWidth * kHeight * kSamplesPerCalculation;
	if((unsigned)glfwGetTime() > last_time)
	{
		last_time = (unsigned)glfwGetTime();
		result = sum;
		sum = 0;
	}

	return result;
}
