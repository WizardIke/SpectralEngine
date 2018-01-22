#pragma once

enum DefaultTextureTypes
{
	diffuse = 0, // contains info on how light is reflected under the surface
	normal, // used to light the surface as if its bumpy
	specularAndRoughness, // contains info on how light is reflected at the surface
	emissive, // used to make some parts of an object glow
	ambientLight, // used for baked lighting, can be very low resolution
	blend, // used for blending different textures together, can be low resolution
};