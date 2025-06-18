#version 430 core

struct BladeIndex {
    float distance;
    uint index;
};

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

layout(std430, binding = 2) buffer UV {
	vec2 uvs[];
};

layout (std430, binding = 4) readonly buffer Sorted {
	BladeIndex sorted[];
};

uniform vec4 		objectColour = vec4(1,1,1,1);

uniform bool hasVertexColours = false;

uniform float maxHeight;

uniform sampler2D perlinWindTex;

uniform vec3 windDir;

uniform bool useWindNoise;

uniform float deltaTime;

uniform bool useFront2Back;

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

vec3 BezierBladeBend(vec3 P0, vec3 P1, vec3 P2, float t){

	

	return pow(1.0 - t, 2.0) * P0 +
               2.0 * (1.0 - t) * t * P1 +
               pow(t, 2.0) * P2;


}

void main(void)
{
	// Get the instance index
	uint sortedIdx = uint(gl_InstanceID);
	uint bladeID = gl_InstanceID.x;
	if (useFront2Back)
		bladeID = sorted[sortedIdx].index;

	mat4 mvp 		  = (projMatrix * viewMatrix * modelMatrix);
	mat3 normalMatrix = transpose ( inverse ( mat3 ( modelMatrix )));

	//OUT.shadowProj 	=  shadowMatrix * vec4 ( position,1);
	//OUT.worldPos 	= ( modelMatrix * vec4 ( position ,1)). xyz ; 
	//OUT.normal 		= normalize ( normalMatrix * normalize ( normal ));
	OUT.colour		= objectColour;


	vec3 worldSpaceOffset = position + positions[bladeID].xyz;
	vec3 localPos = position;

	float randBendAmount = positions[bladeID].w;
	vec2 rotationAmount = rotations[bladeID]; // Get blade rotation amount(radians)


	///////// WIND ///////////////////
	
	if (useWindNoise){

		float windSpeed = windDir.z; // wind speed

		// calc windOffset using wind direction, deltaTime and wind speed
		vec3 windOffset = vec3(windDir.x, 0.0, windDir.y);
							 //x direction		//z direction

	
		float c = rotationAmount.x; // cos()
		float s = rotationAmount.y; // sin()
	
		mat3 bladeRot = mat3(
			vec3( c, 0.0, -s),
			vec3(0.0, 1.0, 0.0),
			vec3( s, 0.0,  c)
		);

		localPos *= bladeRot;

		vec3 bladeDir = normalize(bladeRot * vec3(0.0, 0.0, 1.0) * bladeRot);
		vec3 bendDir = normalize(mix(bladeDir, windOffset, 0.7)); // bias wind

		vec4 windSample = texture(perlinWindTex, uvs[bladeID] + windOffset.xz * deltaTime * windSpeed);
		float windNoise = windSample.r; // get wind noise value

	randBendAmount += windNoise; // add suttle wind noise to bend amount
	OUT.colour *= vec4(windNoise, windNoise, windNoise, 1.0);
		//localPos = BendBladeVertex(localPos, randBendAmount, maxHeight);
	

		// get vertex y position as a factor of maxHeight
		float heightFactor = clamp(localPos.y / maxHeight, 0.0, 1.0);

		vec3 P0 = vec3(0.0, 0.0, 0.0);
		vec3 P2 = vec3(0.0, maxHeight, 0.0);// - windDir * 5  * windNoise; // max pos
		vec3 P1 = mix(P0, P2, 0.5);// + windDir * 4.0 * windNoise ; // control point

		P1 += bendDir * randBendAmount * 0.5; // bend amount
		P2 += bendDir * randBendAmount; // bend amount

		P0 *= bladeRot;
		P2 *= bladeRot;
		P1 *= bladeRot;

		vec3 curved = BezierBladeBend(P0, P1, P2, heightFactor);

		vec3 windDisplacement = (-windOffset * 15) * windSpeed * windNoise * pow(heightFactor, 3.0);

	vec3 bent = curved + windDisplacement;
		localPos += bent;
	}


	// Apply rotation to texture coords
	OUT.texCoord = rotateTex(rotationAmount);

	// Apply shadow gradient closer to base
	OUT.colour *= vec4(localPos, 1.0);

	// apply computed position to vertex
	gl_Position = mvp * vec4(localPos+worldSpaceOffset, 1.0);

}