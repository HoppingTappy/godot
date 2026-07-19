#include "register_types.h"

#include "core/object/class_db.h"
#include "blend_shape_polygon_2d.h"

void initialize_blend_shape_polygon_2d_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_CLASS(BlendShapePolygon2D);
}

void uninitialize_blend_shape_polygon_2d_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
