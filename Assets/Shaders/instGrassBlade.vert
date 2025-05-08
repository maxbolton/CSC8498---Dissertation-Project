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

layout(std430, binding = 1) buffer Rotations {
	vec2 rotations[];
};

uniform vec4 		objectColour = vec4(1,1,1,1);

uniform bool hasVertexColours = false;

uniform float maxHeight;

out Vertex
{
	vec4 colour;
	vec2 texCoord;
	vec4 shadowProj;
	vec3 normal;
	vec3 worldPos;
} OUT;

vec3 BendBladeVertex(vec3 pos, float bendAmount, float maxH) {
    // How far up the blade this vertex is
    float heightFactor = clamp(pos.y / maxH, 0.0, 1.0);

    // Compute a smooth parabolic bend amount based only on height
    float zBend = bendAmount * heightFactor * heightFactor;

    // Output position: same X and Y, new Z
    return vec3(pos.x, pos.y, zBend);
}

vec3 ApplyRotation(vec3 translatedPos, vec2 rotations){
	
	mat2 rotationMat = mat2(rotations.x, -rotations.y,
							rotations.y, rotations.x);

	vec3 rotatedPos;
	rotatedPos.xz = rotationMat * translatedPos.xz;
	rotatedPos.y = translatedPos.y;

	return rotatedPos;
}

vec2 rotateTex(vec2 rotations){

	vec2 rotateTexCoord;
	rotateTexCoord.x = texCoord.x * rotations.x - texCoord.y * rotations.y;
	rotateTexCoord.y = texCoord.x * rotations.y + texCoord.y * rotations.x;
	return rotateTexCoord;
}

void main(void)
{

	mat4 mvp 		  = (projMatrix * viewMatrix * modelMatrix);
	mat3 normalMatrix = transpose ( inverse ( mat3 ( modelMatrix )));

	OUT.shadowProj 	=  shadowMatrix * vec4 ( position,1);
	OUT.worldPos 	= ( modelMatrix * vec4 ( position ,1)). xyz ;
	OUT.normal 		= normalize ( normalMatrix * normalize ( normal ));
	OUT.colour		= objectColour;


	
	// Get per blade bend amount
	float bendVal = positions[gl_InstanceID].w;

	// apply bend to local coords
	vec3 bendBlade = BendBladeVertex(position, bendVal, maxHeight);



	
	// Get blade rotation amount(radians)
	vec2 rotationVal = rotations[gl_InstanceID];

	// Apply rotation to local coords
	vec3 rotated = ApplyRotation(bendBlade , rotationVal);




	// Apply rotation/bend to global coords
	vec3 bladePos = rotated + positions[gl_InstanceID].xyz;

	// Apply rotation to texture coords
	OUT.texCoord = rotateTex(rotationVal);

	gl_Position = mvp * vec4(bladePos, 1.0);

}