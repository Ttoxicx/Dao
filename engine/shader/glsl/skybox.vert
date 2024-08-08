#version 310 es

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

struct DirectionalLight{
	highp vec3 direction;
	lowp float padding_direction;
	highp vec3 color;
	lowp float padding_color;
};

struct PointLight{
	highp vec3 position;
	highp float radius;
	highp vec3 intensity;
	lowp float padding_intensity;
};

layout(set=0,binding=0) readonly buffer skybox_per_frame{
	mat4 proj_view_matrix;
	vec3 camera_position;
	float padding_camera_position;
	vec3 ambient_light;
	float padding_ambient_light;
	uint point_light_num;
	uint padding_point_light_num_1;
	uint padding_point_light_num_2;
	uint padding_point_light_num_3;
	PointLight scene_point_lights[m_max_point_light_count];
	DirectionalLight scene_directional_light;
	highp mat4 directional_light_proj_view;
};

layout(location=0) out vec3 out_uvm;

void main(){
	const vec3 cube_corner_vertex_offsets[8]=vec3[8](
		vec3(1.0,1.0,1.0),
		vec3(1.0,1.0,-1.0),
		vec3(1.0,-1.0,-1.0),
		vec3(1.0,-1.0,1.0),
		vec3(-1.0,1.0,1.0),
		vec3(-1.0,1.0,-1.0),
		vec3(-1.0,-1.0,-1.0),
		vec3(-1.0,-1.0,1.0)
	);

	const int cube_triangle_index[36]=int[36](
		0,1,2,2,3,0,//x+
        4,5,1,1,0,4,//y+ 
        7,6,5,5,4,7,//x-
        3,2,6,6,7,3,//y-
        4,0,3,3,7,4,//z+
        1,5,6,6,2,1 //z-
	);

	vec3 world_position=camera_position+cube_corner_vertex_offsets[cube_triangle_index[gl_VertexIndex]];

	vec4 clip_position=proj_view_matrix*vec4(world_position,1.0);

	clip_position.z=clip_position.w*0.99999;
	gl_Position=clip_position;

	out_uvm=normalize(world_position-camera_position);
}