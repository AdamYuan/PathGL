//
// Created by adamyuan on 4/4/18.
//

#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>

struct RayShader;

class Camera
{
private:
	glm::vec3 origin_, forward_, right_, up_;
	float fov_, yaw_, pitch_, width_, height_, aspect_ratio_;
public:
	explicit Camera();
	void Update();
	void SetLookAt(const glm::vec3 &look_at);
	void SetOrigin(const glm::vec3 &origin);
	void SetFov(const float fov);
	void SetAspectRatio(const float aspect_ratio);
	bool Control(int key);
	void SetUniform(const RayShader &ray) const;
};


#endif
