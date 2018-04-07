//
// Created by adamyuan on 4/4/18.
//

#include <glm/gtc/type_ptr.hpp>
#include "Application.hpp"
#include "Camera.hpp"
#include "Config.hpp"

Camera::Camera()
{
	yaw_ = pitch_ = 0.0f;
	fov_ = 0.3926990817f;
	aspect_ratio_ = 1.0f;
	cam_speed_ = 0.1f;
	cam_angle_ = 20.0f;
}

void Camera::Update()
{
	forward_.x = glm::cos(glm::radians(yaw_)) * glm::cos(glm::radians(pitch_));
	forward_.y = glm::sin(glm::radians(pitch_));
	forward_.z = glm::sin(glm::radians(yaw_)) * glm::cos(glm::radians(pitch_));
	forward_ = glm::normalize(forward_);

	right_ = glm::normalize(glm::cross(forward_, glm::vec3(0.0f, 1.0f, 0.0f)));
	up_ = glm::cross(right_, forward_);

	height_ = glm::tan(fov_);
	width_ = height_ * aspect_ratio_;
}

void Camera::SetLookAt(const glm::vec3 &look_at)
{
	glm::vec3 dir = glm::normalize(look_at - origin_);
	pitch_ = glm::asin(dir.y) * 57.295779513f;
	yaw_ = glm::atan(dir.z, dir.x) * 57.295779513f;
	pitch_ = glm::clamp(pitch_, -89.9f, 89.9f);
}

void Camera::SetOrigin(const glm::vec3 &origin)
{
	origin_ = origin;
}

bool Camera::Control(int key)
{
	if (key == GLFW_KEY_W)
		origin_ += cam_speed_ * forward_;
	else if (key == GLFW_KEY_S)
		origin_ -= cam_speed_ * forward_;
	else if (key == GLFW_KEY_A)
		origin_ -= glm::normalize(glm::cross(forward_, up_)) * cam_speed_;
	else if (key == GLFW_KEY_D)
		origin_ += glm::normalize(glm::cross(forward_, up_)) * cam_speed_;
	else if (key == GLFW_KEY_LEFT)
		yaw_ -= cam_angle_;
	else if (key == GLFW_KEY_RIGHT)
		yaw_ += cam_angle_;
	else if (key == GLFW_KEY_UP)
		pitch_ += cam_angle_;
	else if (key == GLFW_KEY_DOWN)
		pitch_ -= cam_angle_;
	else
		return false;

	pitch_ = glm::clamp(pitch_, -89.9f, 89.9f);

	return true;
}

void Camera::SetUniform(const RayShader &ray) const
{
	glUniform3fv(ray.unif_cam_origin, 1, glm::value_ptr(origin_));
	glUniform3fv(ray.unif_cam_forward, 1, glm::value_ptr(forward_));
	glUniform3fv(ray.unif_cam_up, 1, glm::value_ptr(up_));
	glUniform3fv(ray.unif_cam_right, 1, glm::value_ptr(right_));
	glUniform1f(ray.unif_cam_width, width_);
	glUniform1f(ray.unif_cam_height, height_);
}

void Camera::SetFov(const float fov)
{
	fov_ = fov;
}

void Camera::SetAspectRatio(const float aspect_ratio)
{
	aspect_ratio_ = aspect_ratio;
}

void Camera::SetAngle(float angle)
{
	cam_angle_ = angle;
}

void Camera::SetSpeed(float speed)
{
	cam_speed_ = speed;
}
