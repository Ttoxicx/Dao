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
	//highp float rIndexf=floor(color.r*(_COLORS-1.0));
	//highp float gIndexf=floor(color.g*(_COLORS-1.0));
	//highp float bIndexf=floor(color.b*(_COLORS-1.0));
	//highp float rIndexc=ceil(color.r*(_COLORS-1.0));
	//highp float gIndexc=ceil(color.g*(_COLORS-1.0));
	//highp float bIndexc=ceil(color.b*(_COLORS-1.0));
	//highp float deltax=color.r*(_COLORS-1.0)-rIndexf;
	//highp float deltay=color.g*(_COLORS-1.0)-gIndexf;
	//highp float deltaz=color.b*(_COLORS-1.0)-bIndexf;
	//highp float xf=(rIndexf+bIndexf*_COLORS)/(_COLORS*_COLORS);
	//highp float yf=gIndexf/_COLORS;
	//highp float xc=(rIndexc+bIndexc*_COLORS)/(_COLORS*_COLORS);
	//highp float yc=gIndexc/_COLORS;
	//highp vec4 colorf=texture(color_grading_lut_texture_sampler,vec2(xf,yf));
	//highp vec4 colorc=texture(color_grading_lut_texture_sampler,vec2(xc,yc));
	//out_color=vec4(colorc.x*deltax+colorf.x*(1.0f-deltax),colorc.y*deltay+colorf.y*(1.0f-deltay),colorc.z*deltaz+colorf.z*(1.0f-deltaz),color.a);
	out_color=color;
}