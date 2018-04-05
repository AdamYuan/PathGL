//
// Created by adamyuan on 4/1/18.
//

#include "Application.hpp"
#include "Config.hpp"
#include <fstream>
#include <random>
#include <stb_image_write.h>

std::string Application::read_file(const char *filename)
{
	std::ifstream in(filename);
	return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void Application::check_work_groups()
{
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

	printf("current size x:%u y:%u\ncurrent group count x:%u y:%u\n", UNIT_X, UNIT_Y, GROUP_X, GROUP_Y);
}

void Application::init_window()
{
	gl3wInit();

	glfwInit();
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	window_ = glfwCreateWindow(WIDTH, HEIGHT, "", nullptr, nullptr);
	glfwMakeContextCurrent(window_);

	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("renderer: %s\n", glGetString(GL_RENDERER));
	printf("vendor: %s\n", glGetString(GL_VENDOR));

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
	glBindImageTexture(0, res_.quad_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	std::mt19937 generator;
	auto *initial_seed = new GLuint[WIDTH * HEIGHT * 4];
	for(int i = 0; i < WIDTH * HEIGHT * 4; ++i)
		initial_seed[i] = (GLuint)(generator());
	glGenTextures(1, &ray_.seed_tex);
	glBindTexture(GL_TEXTURE_2D, ray_.seed_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, WIDTH, HEIGHT, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, initial_seed);
	glBindImageTexture(1, ray_.seed_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32UI);

	delete[] initial_seed;
}

void Application::init_ray_shader()
{
	std::string ray_shader_str {read_file(RAY_SHADER)};
	const char *ray_shader_src1 = ray_shader_str.c_str();
	char ray_shader_src[65536];
	sprintf(ray_shader_src, "#version 430 core\nlayout(local_size_x = %u, local_size_y = %u) in;\n%s", UNIT_X, UNIT_Y, ray_shader_src1);

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
	glUniform1i(ray_.unif_last_sample, samples_);
	glDispatchCompute(GROUP_X, GROUP_Y, 1);
	// make sure writing to image has finished before read
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	samples_ ++;
}

Application::Application() : cam_(0.3926990817f * 2.0f)
{
	samples_ = 0;
	locked_ = false;
	init_window();
	init_buffers();
	init_texture();
	init_quad_shader();
	init_ray_shader();
	check_work_groups();

	cam_.SetOrigin({.0f, .0f, .0f});
	cam_.SetLookAt({.0f, .0f, -1.0f});

	//cam_.SetOrigin({50.0f, 45.0f, 205.6f});
	//cam_.SetLookAt({50.0f, 44.9f, 204.6f});
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

		sprintf(title, "%d pass %luk sp/s %s", samples_, get_sps() / 1000lu, locked_ ? " [locked]" : "");
		glfwSetWindowTitle(window_, title);
	}
}

void Application::screenshot()
{
	auto *pixels = new uint8_t[WIDTH * HEIGHT * 3];
	auto *final = new uint8_t[WIDTH * HEIGHT * 3];

	glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	for(int i = 0; i < HEIGHT; i ++)
		std::copy(pixels + i * WIDTH * 3, pixels + (i + 1) * WIDTH * 3, final + (HEIGHT - 1 - i) * WIDTH * 3);

	char filename[256];
	sprintf(filename, "%dspp.png", samples_);
	stbi_write_png(filename, WIDTH, HEIGHT, 3, final, 0);

	delete[] pixels;
	delete[] final;
}


void Application::key_callback(GLFWwindow *window, int key, int, int action, int)
{
	auto *app = (Application*)glfwGetWindowUserPointer(window);
	if(action == GLFW_PRESS)
	{
		if(!app->locked_ && app->cam_.Control(key))
			app->samples_ = 0;
		else if(key == GLFW_KEY_I)
			app->screenshot();
		else if(key == GLFW_KEY_L)
			app->locked_ = !app->locked_;
	}
}

unsigned long Application::get_sps()
{
	static unsigned last_time = 0;
	static unsigned long result = 0, sum = 0;

	sum += WIDTH * HEIGHT;
	if((unsigned)glfwGetTime() > last_time)
	{
		last_time = (unsigned)glfwGetTime();
		result = sum;
		sum = 0;
	}

	return result;
}
