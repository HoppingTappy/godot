#include "register_types.h"

#include "core/object/class_db.h"
#include "blend_shape_mesh_instance_2d.h"

void initialize_blend_shape_mesh_instance_2d_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_CLASS(BlendShapeMeshInstance2D);
}

void uninitialize_blend_shape_mesh_instance_2d_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
