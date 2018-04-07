//
// Created by adamyuan on 4/7/18.
//

#include "SceneLoader.hpp"
#include "Camera.hpp"
#include <fstream>
#include <sstream>


SceneLoader::SceneLoader(const char *filename)
{
	std::ifstream in{filename};
	if(!in.is_open())
		throw std::runtime_error("file not found");
	std::string buf, cmd;
	int line{0};
	while(std::getline(in, buf))
	{
		line ++;
		if(buf.empty() || buf[0] == '#') //comment
			continue;
		std::istringstream ss(buf);
		ss >> cmd;
		if(cmd == "cam_pos")
			ss >> cam_pos_.x >> cam_pos_.y >> cam_pos_.z;
		else if(cmd == "cam_look_at")
			ss >> cam_look_at_.x >> cam_look_at_.y >> cam_look_at_.z;
		else if(cmd == "plane")
		{
			Plane p;
			ss >>
			   p.position.x >> p.position.y >> p.position.z >> //position
			   p.normal.x >> p.normal.y >> p.normal.z >> //normal
			   p.color.r >> p.color.g >> p.color.b >> //color
			   p.emission.r >> p.emission.g >> p.emission.b >> //emission
			   p.type; //type
			if(p.type != "DIFFUSE" && p.type != "SPECULAR" && p.type != "REFRACTIVE")
				throw std::runtime_error("invalid material at line " + std::to_string(line));
			planes_.push_back(p);
		}
		else if(cmd == "sphere")
		{
			Sphere s;
			ss >>
			   s.center.x >> s.center.y >> s.center.z >> //center
			   s.radius >> //radius
			   s.color.r >> s.color.g >> s.color.b >> //color
			   s.emission.r >> s.emission.g >> s.emission.b >> //emission
			   s.type; //type
			if(s.type != "DIFFUSE" && s.type != "SPECULAR" && s.type != "REFRACTIVE")
				throw std::runtime_error("invalid material at line " + std::to_string(line));
			spheres_.push_back(s);
		}
		else
			throw std::runtime_error("invalid command " + cmd + " at line " + std::to_string(line));
	}
	if(cam_look_at_ == cam_pos_)
		throw std::runtime_error("invalid camera definition");
}

std::string SceneLoader::GetGlsl() const
{
	std::string ret {
			"#define DIFFUSE 0\n"
			"#define SPECULAR 1\n"
			"#define REFRACTIVE 2\n"
			"struct Material\n"
			"{\n"
			"	vec3 color, emission;\n"
			"	int type;\n"
			"};\n"
			"struct Plane\n"
			"{\n"
			"	vec3 position, normal;\n"
			"	Material material;\n"
			"};\n"
			"struct Sphere\n"
			"{\n"
			"	vec3 center;\n"
			"	float radius;\n"
			"	Material material;\n"
			"};\n" };
	ret += "#define PLANE_NUM " + std::to_string(planes_.size()) +
		   "\n#define SPHERE_NUM " + std::to_string(spheres_.size()) + "\n";
	ret += "const Plane planes[PLANE_NUM] = { ";
	{
		for(const Plane &p : planes_)
		{
			ret += "{";
			ret += vec3_to_string(p.position) + "," + vec3_to_string(p.normal) + ",";
			ret += "{";
			ret += vec3_to_string(p.color) + "," + vec3_to_string(p.emission) + "," + p.type;
			ret += "}},";
		}
		ret.pop_back();
	}
	ret += "};\n";

	ret += "const Sphere spheres[SPHERE_NUM] = { ";
	{
		for(const Sphere &s : spheres_)
		{
			ret += "{";
			ret += vec3_to_string(s.center) + "," + std::to_string(s.radius) + ",";
			ret += "{";
			ret += vec3_to_string(s.color) + "," + vec3_to_string(s.emission) + "," + s.type;
			ret += "}},";
		}
		ret.pop_back();
	}
	ret += "};\n";

	return ret;
}

void SceneLoader::SetCamera(Camera *cam) const
{
	cam->SetOrigin(cam_pos_);
	cam->SetLookAt(cam_look_at_);
}

