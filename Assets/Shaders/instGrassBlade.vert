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

	//OUT.colour = vec4(0.0, 1.0, 0.0, 1.0);

	// derive rotation from w component //

	vec3 bladePos = positions[gl_InstanceID].xyz;
	float bladeYaw = positions[gl_InstanceID].w;

	float cosYaw = cos(bladeYaw);
	float sinYaw = sin(bladeYaw);

	mat2 rotationMat = mat2(
	cosYaw, -sinYaw,
	sinYaw, cosYaw
	);

	// apply rotation to local position
	vec2 rotatedOffset = rotationMat * position.xz;

	vec3 worldPos = bladePos + vec3(rotatedOffset.x, position.y, rotatedOffset.y);

	vec2 rotatedTexCoord;
	rotatedTexCoord.x = texCoord.x * cosYaw - texCoord.y * sinYaw;
	rotatedTexCoord.y = texCoord.x * sinYaw + texCoord.y * cosYaw;

	OUT.texCoord = rotatedTexCoord;


	gl_Position = mvp * vec4(worldPos, 1.0);

}