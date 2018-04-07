//
// Created by adamyuan on 4/7/18.
//

#ifndef SCENELOADER_HPP
#define SCENELOADER_HPP


#include <string>
#include <vector>
#include <glm/glm.hpp>

struct Plane
{
	//position normal color emission type
	glm::vec3 position, normal, color, emission;
	std::string type;
};
struct Sphere
{
	//origin radius color emission type
	glm::vec3 center;
	float radius;
	glm::vec3 color, emission;
	std::string type;
};
struct Camera;
class SceneLoader
{
private:
	std::vector<Plane> planes_;
	std::vector<Sphere> spheres_;
	glm::vec3 cam_pos_, cam_look_at_;
	static std::string vec3_to_string(const glm::vec3 &v)
	{
		return "{" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + "}";
	}

public:
	explicit SceneLoader(const char *filename);
	std::string GetGlsl() const;
	void SetCamera(Camera *cam) const;
};


#endif //SCENELOADER_HPP
