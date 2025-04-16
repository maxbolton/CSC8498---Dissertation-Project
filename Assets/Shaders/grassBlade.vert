#version 400 core

uniform mat4 modelMatrix 	= mat4(1.0f);
uniform mat4 viewMatrix 	= mat4(1.0f);
uniform mat4 projMatrix 	= mat4(1.0f);
uniform mat4 shadowMatrix 	= mat4(1.0f);

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 colour;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 normal;

uniform vec4 		objectColour = vec4(1,1,1,1);

uniform bool hasVertexColours = false;

uniform float rand = -999;
uniform float bendHeight = 1;

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

	if (rand == -999) {
		OUT.colour = vec4(1,0,0,1);
	}

	// Apply bending effect
	vec3 bentPosition = position;
	if (position.y > bendHeight) {
		float bendFactor = (position.y - bendHeight) / (1.0 - bendHeight); // Normalize bend factor
		bentPosition.x += sin((position.y - bendHeight) * 3.14159) * rand; // Adjust the axis and amount of bending as needed
	}

	gl_Position		= mvp * vec4(bentPosition, 1.0);
}