#version 310 es

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(location=0) flat in highp uint in_nodeid;

layout(early_fragment_tests) in;

layout(location=0) out highp uint out_nodeid;

void main(){
	out_nodeid=in_nodeid;
}