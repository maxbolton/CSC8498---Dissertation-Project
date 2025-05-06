#version 430 core

uniform mat4 modelMatrix 	= mat4(1.0f);
uniform mat4 viewMatrix 	= mat4(1.0f);
uniform mat4 projMatrix 	= mat4(1.0f);
uniform mat4 shadowMatrix 	= mat4(1.0f);

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 colour;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 normal;

layout(std430, binding = 0) buffer Positions {
	vec4 positions[];
};

uniform vec4 		objectColour = vec4(1,1,1,1);

uniform bool hasVertexColours = false;

out Vertex
{
	vec4 colour;
	vec2 texCoord;
	vec4 shadowProj;
	vec3 normal;
	vec3 worldPos;
} OUT;


void main(void)
{

	mat4 mvp 		  = (projMatrix * viewMatrix * modelMatrix);
	mat3 normalMatrix = transpose ( inverse ( mat3 ( modelMatrix )));

	OUT.shadowProj 	=  shadowMatrix * vec4 ( position,1);
	OUT.worldPos 	= ( modelMatrix * vec4 ( position ,1)). xyz ;
	OUT.normal 		= normalize ( normalMatrix * normalize ( normal ));
	
	OUT.texCoord	= texCoord;
	OUT.colour		= objectColour;

	if(hasVertexColours) {
		OUT.colour		= objectColour * colour;
	}

	OUT.colour = vec4(0.0, 1.0, 0.0, 1.0);

	vec3 offset = positions[gl_InstanceID].xyz;
	
	float rotDeg = positions[gl_InstanceID].w;
	float rotRad = radians(rotDeg);

	float cosR = cos(rotRad);
	float sinR = sin(rotRad);

	mat2 rotationMat = mat2(
	cosR, -sinR,
	sinR, cosR
	);

	vec2 rotatedXZ = rotationMat * position.xz;

	vec4 worldPos = vec4(rotatedXZ.x + offset.x, position.y + offset.y, rotatedXZ.y + offset.z, 1.0);



	gl_Position = mvp * worldPos;

}