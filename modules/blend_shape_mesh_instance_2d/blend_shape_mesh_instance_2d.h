/**************************************************************************/
/*  blend_shape_mesh_instance_2d.h                                       */
/**************************************************************************/

#ifndef BLEND_SHAPE_MESH_INSTANCE_2D_H
#define BLEND_SHAPE_MESH_INSTANCE_2D_H

#include "scene/2d/mesh_instance_2d.h"
#include "scene/resources/mesh.h"
#include "core/templates/hash_map.h"

class BlendShapeMeshInstance2D : public MeshInstance2D {
	GDCLASS(BlendShapeMeshInstance2D, MeshInstance2D);

private:
	Vector<float> blend_shape_tracks;

	Ref<Mesh> source_mesh;
	Ref<ArrayMesh> display_mesh;
	bool updating_mesh = false;

	void _mesh_changed();

protected:
	static void _bind_methods();
	void _notification(int p_what);

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _property_can_revert(const StringName &p_name) const;
	bool _property_get_revert(const StringName &p_name, Variant &r_property) const;

public:
	void set_mesh(const Ref<Mesh> &p_mesh);

	int get_blend_shape_count() const;
	int find_blend_shape_by_name(const StringName &p_name);
	float get_blend_shape_value(int p_blend_shape) const;
	void set_blend_shape_value(int p_blend_shape, float p_value);

	Ref<ArrayMesh> bake_mesh_from_current_blend_shape_mix(Ref<ArrayMesh> p_existing = Ref<ArrayMesh>());

	BlendShapeMeshInstance2D();
	~BlendShapeMeshInstance2D();
};

#endif // BLEND_SHAPE_MESH_INSTANCE_2D_H