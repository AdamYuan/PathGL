layout(rgba32f, binding = 0) uniform image2D out_img;
layout(rgba32ui, binding = 1) uniform uimage2D seed_img;

uniform vec3 cam_origin, cam_forward, cam_right, cam_up;
uniform float cam_width, cam_height;
uniform int last_sample;

#define PIXEL_COORDS ivec2(gl_GlobalInvocationID.xy)
#define MAX_DEPTH 8
#define RAY_T_MIN 0.0001f
#define RAY_T_MAX 1.0e30f
const float REFR_INDEX = 1.5f;
const float REV_REFR_INDEX = 1.0f / REFR_INDEX;
const float R0 = ((1.0f - REFR_INDEX) / (1.0f + REFR_INDEX)) * ((1.0f - REFR_INDEX) / (1.0f + REFR_INDEX));

//randomness solution
//https://math.stackexchange.com/questions/337782/pseudo-random-number-generation-on-the-gpu
uint taus_step(uint z, int S1, int S2, int S3, uint M)
{
	uint b = (((z << S1) ^ z) >> S2);
	return (((z & M) << S3) ^ b);
}
uint lcg_step(uint z, uint A, uint C)
{
	return (A * z + C);
}
float rand()
{
	uvec4 seed = imageLoad(seed_img, PIXEL_COORDS);
	seed.x = taus_step(seed.x, 13, 19, 12, 4294967294);
	seed.y = taus_step(seed.y, 2, 25, 4, 4294967288);
	seed.z = taus_step(seed.z, 3, 11, 17, 4294967280);
	seed.w = lcg_step(seed.w, 1664525, 1013904223);
	imageStore(seed_img, PIXEL_COORDS, seed);
	return float(seed.x ^ seed.y ^ seed.z ^ seed.w) * 2.3283064365387e-10f;
}
//////////////////////

struct Ray
{
	vec3 origin, direction;
};
#define DIFFUSE 0
#define SPECULAR 1
#define REFRACTIVE 2
struct Material
{
	vec3 emission, color;
	int type;
};
struct Sphere
{
	bool inside;
	float radius;
	vec3 center;
	Material material;
};

#define SPHERE_NUM 9

const Sphere spheres[SPHERE_NUM] =
{
	{true,	10000,	{10001, 40.8, 81.6	},		{{0, 0, 0},		{0.75, .25, 0.25},	DIFFUSE}},
	{true,	10000,	{-9901, 40.8, 81.6	},		{{0, 0, 0},		{0.25, .25, 0.75},	DIFFUSE}},
	{true,	10000,	{50, 40.8, 10000	},		{{0, 0, 0},		{0.75, .75, 0.75},	DIFFUSE}},
	{true,	10000,	{50, 40.8, -9730	},		{{0, 0, 0},		{0, 0, 0,		},	DIFFUSE}},
	{true,	10000,	{50, 10000, 81.6	},		{{0, 0, 0},		{0.75, .75, 0.75},	DIFFUSE}},
	{true,	10000,	{50, -9918.4, 81.6	},		{{0, 0, 0},		{0.75, .75, 0.75},	DIFFUSE}},
	{false,	16.5,	{27, 16.5, 47		},		{{0, 0, 0},		{0.9, 0.9, 0.9	},	SPECULAR}},
	{false,	16.5,	{73, 16.5, 78		},		{{0, 0, 0},		{0.9, 0.9, 0.9	},	REFRACTIVE}},
	{false,	7,		{50, 66.6, 81.6		},		{{50, 50, 50},	{0, 0, 0		},	DIFFUSE}}
};

float sphere_intersect(in const Sphere sphere, in const Ray ray, in const float t_max)
{
	Ray local_ray = Ray(ray.origin - sphere.center, ray.direction);

	float b = 2.0f * dot(local_ray.direction, local_ray.origin);
	float c = dot(local_ray.origin, local_ray.origin) - sphere.radius * sphere.radius;

	float delta = b*b - 4*c;
	if(delta <= 0.0f)
		return RAY_T_MAX;

	delta = sqrt(delta);

	float t1 = (-b - delta) / 2.0f;
	if(t1 > RAY_T_MIN && t1 < t_max)
		return t1;

	float t2 = (-b + delta) / 2.0f;
	if(t2 > RAY_T_MIN && t2 < t_max)
		return t2;

	return RAY_T_MAX;
}

bool intersect(in const Ray ray, out vec3 p, out vec3 normal, out Material mat)
{
	float t = RAY_T_MAX, new_t;
	int id = -1;
	for(int i = 0; i < SPHERE_NUM; ++i)
	{
		new_t = sphere_intersect(spheres[i], ray, t);
		if(new_t < t) { t = new_t; id = i; }
	}
	if(id != -1) //hit
	{
		p = ray.origin + ray.direction * t;
		normal = normalize(p - spheres[id].center);
		if(spheres[id].inside)
			normal = -normal;
		mat = spheres[id].material;
		return true;
	}
	return false;
}

vec3 hemisphere(in const float u1, in const float u2)
{
	float r = sqrt(1.0f - u1 * u1), phi = 6.28318530718f * u2;
	return vec3(cos(phi) * r, sin(phi) * r, u1);
}

vec3 trace(in Ray ray)
{
	vec3 ret = vec3(0.0f), k = vec3(1.0f, 1.0f, 1.0f);
	vec3 normal, nl, p;
	Material mat;
	for(int depth = 0; depth < MAX_DEPTH; ++depth)
	{
		if(!intersect(ray, p, normal, mat))
			break;

		ray.origin = p;
		ret += mat.emission * k;

		if(mat.type == DIFFUSE)
		{
			vec3 rot_x, rot_y;
			if(abs(normal.x) > abs(normal.y)) {
				float inv_len = 1.0f / length(normal.xz);
				rot_x = vec3(-normal.z * inv_len, 0.0f, normal.x * inv_len);
			} else {
				float inv_len = 1.0f / length(normal.yz);
				rot_x = vec3(0.0f, normal.z * inv_len, -normal.y * inv_len);
			}
			rot_y = cross(normal, rot_x);

			vec3 hemi_dir = hemisphere(rand(), rand());

			ray.direction =
				vec3 (
						dot(vec3(rot_x.x, rot_y.x, normal.x), hemi_dir),
						dot(vec3(rot_x.y, rot_y.y, normal.y), hemi_dir),
						dot(vec3(rot_x.z, rot_y.z, normal.z), hemi_dir)
					 );

			float cost = dot(ray.direction, normal);
			k *= mat.color * cost;
		}
		else if(mat.type == SPECULAR)
		{
			float cost = dot(ray.direction, normal);
			ray.direction = normalize(ray.direction - normal * cost * 2.0f); //reflect
			k *= mat.color;
		}
		else //refractive
		{
			float refr_ind = REV_REFR_INDEX;

			float cost1 = -dot(normal, ray.direction);
			if(cost1 < 0)//inside
			{
				normal = -normal;
				cost1 = -cost1;
				refr_ind = REFR_INDEX;
			}

			float cost2 = 1.0f - refr_ind * refr_ind * (1.0f - cost1 * cost1);
			float r_prob = R0 + (1.0f + R0) * pow(1.0f - cost1, 5.0f);
			if(cost2 > 0 && rand() > r_prob)
				ray.direction = normalize(ray.direction * refr_ind + normal * (refr_ind * cost1 - sqrt(cost2)));
			else
				ray.direction = normalize(ray.direction + normal * cost1 * 2.0f);

			k *= mat.color * 1.15f;
		}
	}
	return ret;
}

Ray make_ray(in const vec2 screen_pos)
{
	vec2 size = vec2(imageSize(out_img));
	vec2 pos = 2.0f * screen_pos / size - 1.0f;
	vec3 dir = normalize(cam_forward + (cam_right * pos.x * cam_width) + (cam_up * pos.y * cam_height));

	return Ray(cam_origin, dir);
}

void main()
{
	vec3 color = imageLoad(out_img, PIXEL_COORDS).xyz;
	vec3 new = trace(make_ray(vec2(gl_GlobalInvocationID.xy) + vec2(rand(), rand()) * 2.0f - 1.0f));
	color = (color * float(last_sample) + new) / float(last_sample + 1);
	imageStore(out_img, PIXEL_COORDS, vec4(color, 1.0f));
}
