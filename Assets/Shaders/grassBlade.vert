#version 450 core

struct GrassBlade {
		int id;
		vec3 position;
		vec3 faceRotation;
		float bendAmount;
		float noiseValue;
	};

layout(std430, binding = 0) buffer GrassBladeBuffer {
	GrassBlade blades[];
};

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

uniform float bendAmount = -999;
uniform float maxHeight = -1;
uniform float bendHeightPercent = .6;
uniform float noiseAmount = -999;


out Vertex
{
	vec4 colour;
	vec2 texCoord;
	vec4 shadowProj;
	vec3 normal;
	vec3 worldPos;
} OUT;

bool isBackFace(){ return dot(normal, vec3(0, 0, 1)) < 0.0; }

vec3 BendBladeVertex(vec3 pos, float bendAmount, float maxHeight) {
    // How far up the blade this vertex is
    float heightFactor = clamp(pos.y / maxHeight, 0.0, 1.0);

    // Compute a smooth parabolic bend amount based only on height
    float zBend = bendAmount * heightFactor * heightFactor;

    // Output position: same X and Y, new Z
    return vec3(pos.x, pos.y, zBend);
}


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
	
	vec3 bentPosition = position;

	if (bendAmount != -999.0 && maxHeight != -1.0) {
		bentPosition = BendBladeVertex(position, bendAmount, maxHeight);

		float h = clamp(position.y/maxHeight, 0.0, 1.0);
		// dz/dy = 2 * bendAmount * h / maxHeight
		float slope = 2.0 * bendAmount * h / maxHeight;
		vec3 bentNormal = normalize(vec3(0.0, -slope, 1.0));

		OUT.normal = normalize(normalMatrix * bentNormal);

		vec4 worldPos4 = modelMatrix * vec4(bentPosition, 1.0);
		OUT.worldPos   = worldPos4.xyz;
		OUT.shadowProj = shadowMatrix * vec4(bentPosition, 1.0);
		gl_Position    = projMatrix * viewMatrix * worldPos4;

		
		OUT.colour.x = noiseAmount;

	}
	else {
		OUT.colour = vec4(1,0,0,1);
		gl_Position = mvp * vec4(position, 1.0);
	}

}