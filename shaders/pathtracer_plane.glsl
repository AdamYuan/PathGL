layout(rgba32f, binding = 0) uniform image2D out_img;
layout(rgba32ui, binding = 1) uniform uimage2D seed_img;

uniform vec3 cam_origin, cam_forward, cam_right, cam_up;
uniform float cam_width, cam_height;
uniform uint last_sample;

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

#define NONE -1
#define SPHERE 0
#define PLANE 1
float plane_intersect(in const Plane plane, in const Ray ray, in const float t_max)
{
	float d_dot_n = dot(ray.direction, plane.normal);
	if(d_dot_n == 0.0f)
		return RAY_T_MAX;

	float t = dot(plane.position - ray.origin, plane.normal) / d_dot_n;

	if(t <= RAY_T_MIN || t >= t_max)
		return RAY_T_MAX;

	return t;
}

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
	int type = NONE, id;
	for(int i = 0; i < PLANE_NUM; ++i)
	{
		new_t = plane_intersect(planes[i], ray, t);
		if(new_t < t) { t = new_t; type = PLANE; id = i; }
	}
	for(int i = 0; i < SPHERE_NUM; ++i)
	{
		new_t = sphere_intersect(spheres[i], ray, t);
		if(new_t < t) { t = new_t; type = SPHERE; id = i; }
	}

	if(type != NONE)
	{
		p = ray.origin + ray.direction * t;
		if(type == PLANE)
		{
			normal = planes[id].normal;
			mat = planes[id].material;
		}
		else //sphere
		{
			normal = normalize(p - spheres[id].center);
			mat = spheres[id].material;
		}
		return true;
	}
	return false;
}

/*vec3 hemisphere(in const float u1, in const float u2)
{
	float r = sqrt(1.0f - u1 * u1), phi = 6.28318530718f * u2;
	return vec3(cos(phi) * r, sin(phi) * r, u1);
}*/

vec3 trace(in Ray ray)
{
	vec3 ret = vec3(0.0f), k = vec3(1.0f, 1.0f, 1.0f);
	vec3 p, normal;
	Material mat;
	for(int depth = 0; depth < MAX_DEPTH; ++depth)
	{
		if(intersect(ray, p, normal, mat))
		{
			ray.origin = p;
			ret += mat.emission * k;
			k *= mat.color;

			if(mat.type == DIFFUSE)
			{
				//pick random reflect ray (from smallpt)
				float r1 = rand() * 6.28318530718f;
				float r2 = rand(), r2s = sqrt(r2);
				vec3 u = normalize(cross(abs(normal.x) > .01 ? vec3(0, 1, 0) : vec3(1, 0, 0), normal));
				vec3 v = cross(normal, u);
				ray.direction = normalize(u * cos(r1) * r2s + v * sin(r1) * r2s + normal * sqrt(1 - r2));

				float cos_t = dot(ray.direction, normal);
				k *= cos_t;
			}
			else if(mat.type == SPECULAR)
				ray.direction = reflect(ray.direction, normal);
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

				k *= 1.15f;
				/*vec3 refl_dir = reflect(ray.direction, normal);
				float ddn = dot(normal, ray.direction);
				bool into = ddn > 0;
				if(!into) ddn = -ddn;
				float nnt = into ? REV_REFR_INDEX : REFR_INDEX;
				float cos_t2;
				if((cos_t2 = 1.0f - nnt*nnt * (1.0f - ddn*ddn)) < 0)
				{
					ray.direction = refl_dir;
					continue;
				}
				vec3 tdir = normalize(ray.direction*nnt - normal * ((into?1:-1) * (ddn*nnt+sqrt(cos_t2))));
				float c = 1.0f - (into ? -ddn : dot(tdir, normal));
				float Re = R0 + (1.0f - R0) * pow(c, 5.0f);
				float Tr = 1.0f - Re;
				float P = 0.25f + 0.5f * Re;
				float RP = Re / P, TP = Tr / (1.0f - P);
				if(rand() < P)
				{
					ray.direction = refl_dir;
					k *= RP;
				}
				else
				{
					ray.direction = tdir;
					k *= TP;
				}*/
			}
		}
		else
			break;
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
	vec3 color = imageLoad(out_img, PIXEL_COORDS).xyz * float(last_sample);
	for(int i = 0; i < SAMPLES; ++i)
	{
		Ray cam_ray = make_ray(vec2(gl_GlobalInvocationID.xy) + vec2(rand() - 0.5f, rand() - 0.5f));
		color += trace(cam_ray);
	}
	color /= float(last_sample + SAMPLES);

	imageStore(out_img, PIXEL_COORDS, vec4(color, 1.0f));
}
