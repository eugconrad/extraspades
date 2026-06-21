/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

varying vec4 color;
varying vec2 ambientOcclusionCoord;
varying vec2 textureCoord;
varying float blockDamage;
varying vec3 fogDensity;

uniform sampler2D ambientOcclusionTexture;
uniform sampler2D blockTextureAtlas;
uniform int texturedBlocksEnabled;
uniform float texturedBlockBrightness;
uniform float texturedBlockTint;
uniform float texturedBlockDamage;
uniform vec3 fogColor;

vec3 EvaluateSunLight();
vec3 EvaluateAmbientLight(float detailAmbientOcclusion);

void main() {
	// color is linear
	gl_FragColor = vec4(color.xyz, 1.0);
	if (texturedBlocksEnabled != 0) {
		gl_FragColor.xyz = mix(vec3(1.0), color.xyz, texturedBlockTint);
		vec3 textureColor = texture2D(blockTextureAtlas, textureCoord).xyz;
		textureColor = mix(textureColor, vec3(0.5), clamp(blockDamage * texturedBlockDamage, 0.0, 1.0));
		textureColor *= textureColor; // linearize
		textureColor *= texturedBlockBrightness;
		gl_FragColor.xyz *= textureColor;
	}

	float ao = texture2D(ambientOcclusionTexture, ambientOcclusionCoord).x;
	vec3 diffuseShading = EvaluateAmbientLight(ao);
	diffuseShading += vec3(color.w) * EvaluateSunLight();
	
	// apply diffuse shading
	gl_FragColor.xyz *= diffuseShading;

	// apply fog fading
	gl_FragColor.xyz = mix(gl_FragColor.xyz, fogColor, fogDensity);

#if !LINEAR_FRAMEBUFFER
	// gamma correct
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#endif
}
