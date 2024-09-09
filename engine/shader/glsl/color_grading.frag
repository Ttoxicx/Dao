#version 310 es

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

//get color data from previous subpass,input_attachment_index to make data transfer successfully
layout(input_attachment_index=0,set=0,binding=0) uniform highp subpassInput in_color;

layout(set=0,binding=1) uniform sampler2D color_grading_lut_texture_sampler;

layout(location=0) out highp vec4 out_color;

void main(){
	highp vec4 color=subpassLoad(in_color).rgba;
	//highp ivec2 lut_tex_size=textureSize(color_grading_lut_texture_sampler,0);
	//highp float _COLORS=float(lut_tex_size.y);
	//highp float rIndex = floor(color.r * (_COLORS-1.0));
	//highp float gIndex = floor(color.g * (_COLORS-1.0));
	//highp float bIndex = floor(color.b * (_COLORS-1.0));
	//highp float x = (rIndex + bIndex * _COLORS) / (_COLORS*_COLORS);
	//highp float y = gIndex / _COLORS;
	//out_color=texture(color_grading_lut_texture_sampler,vec2(x,y));
	out_color=color;
}