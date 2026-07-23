/**************************************************************************/
/*  blend_shape_mesh_instance_2d.cpp                                     */
/**************************************************************************/

#include "blend_shape_mesh_instance_2d.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"

BlendShapeMeshInstance2D::BlendShapeMeshInstance2D() {}
BlendShapeMeshInstance2D::~BlendShapeMeshInstance2D() {}

void BlendShapeMeshInstance2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_POSTINITIALIZE: {
			if (!updating_mesh && source_mesh.is_null()) {
				Ref<Mesh> current = MeshInstance2D::get_mesh();
				if (current.is_valid()) {
					set_mesh(current);
				}
			}
		} break;
	}
}

void BlendShapeMeshInstance2D::set_mesh(const Ref<Mesh> &p_mesh) {
	if (updating_mesh) {
		MeshInstance2D::set_mesh(p_mesh);
		return;
	}

	if (source_mesh == p_mesh && source_mesh.is_valid()) {
		return;
	}

	if (source_mesh.is_valid()) {
		source_mesh->disconnect_changed(callable_mp(this, &BlendShapeMeshInstance2D::_mesh_changed));
	}

	source_mesh = p_mesh;

	if (source_mesh.is_valid()) {
		source_mesh->connect_changed(callable_mp(this, &BlendShapeMeshInstance2D::_mesh_changed));
		_mesh_changed();
	} else {
		blend_shape_tracks.clear();
		display_mesh.unref();

		updating_mesh = true;
		MeshInstance2D::set_mesh(Ref<Mesh>());
		updating_mesh = false;
	}

	notify_property_list_changed();
	update_configuration_warnings();
}

void BlendShapeMeshInstance2D::_mesh_changed() {
	if (updating_mesh || source_mesh.is_null()) {
		return;
	}

	uint32_t initialize_bs_from = blend_shape_tracks.size();
	blend_shape_tracks.resize(source_mesh->get_blend_shape_count());

	for (uint32_t i = initialize_bs_from; i < blend_shape_tracks.size(); i++) {
		blend_shape_tracks.write[i] = 0.0f;
	}

	display_mesh = bake_mesh_from_current_blend_shape_mix(display_mesh);

	updating_mesh = true;
	MeshInstance2D::set_mesh(display_mesh);
	updating_mesh = false;

	notify_property_list_changed();
}

bool BlendShapeMeshInstance2D::_set(const StringName &p_name, const Variant &p_value) {
	String name = p_name;
	if (name.begins_with("blend_shapes/")) {
		String shape_name = name.substr(13);
		int idx = find_blend_shape_by_name(shape_name);
		if (idx != -1) {
			set_blend_shape_value(idx, p_value);
			return true;
		}
	}
	return false;
}

bool BlendShapeMeshInstance2D::_get(const StringName &p_name, Variant &r_ret) const {
	String name = p_name;
	if (name.begins_with("blend_shapes/")) {
		String shape_name = name.substr(13);
		if (source_mesh.is_valid()) {
			for (int i = 0; i < source_mesh->get_blend_shape_count(); i++) {
				if (source_mesh->get_blend_shape_name(i) == shape_name) {
					r_ret = get_blend_shape_value(i);
					return true;
				}
			}
		}
	}
	return false;
}

void BlendShapeMeshInstance2D::_get_property_list(List<PropertyInfo> *p_list) const {
	if (source_mesh.is_valid()) {
		int count = source_mesh->get_blend_shape_count();
		for (int i = 0; i < count; i++) {
			String shape_name = source_mesh->get_blend_shape_name(i);
			p_list->push_back(PropertyInfo(
				Variant::FLOAT,
				vformat("blend_shapes/%s", shape_name),
				PROPERTY_HINT_RANGE,
				"-1,1,0.00001",
				PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE
			));
		}
	}
}

bool BlendShapeMeshInstance2D::_property_can_revert(const StringName &p_name) const {
	String name = p_name;
	return name.begins_with("blend_shapes/");
}

bool BlendShapeMeshInstance2D::_property_get_revert(const StringName &p_name, Variant &r_property) const {
	String name = p_name;
	if (name.begins_with("blend_shapes/")) {
		r_property = 0.0f;
		return true;
	}
	return false;
}

int BlendShapeMeshInstance2D::get_blend_shape_count() const {
	if (source_mesh.is_null()) {
		return 0;
	}
	return source_mesh->get_blend_shape_count();
}

int BlendShapeMeshInstance2D::find_blend_shape_by_name(const StringName &p_name) {
	if (source_mesh.is_null()) {
		return -1;
	}
	for (int i = 0; i < source_mesh->get_blend_shape_count(); i++) {
		if (source_mesh->get_blend_shape_name(i) == p_name) {
			return i;
		}
	}
	return -1;
}

float BlendShapeMeshInstance2D::get_blend_shape_value(int p_blend_shape) const {
	if (source_mesh.is_null()) {
		return 0.0f;
	}
	if (p_blend_shape < 0 || p_blend_shape >= blend_shape_tracks.size()) {
		return 0.0f;
	}
	return blend_shape_tracks[p_blend_shape];
}

void BlendShapeMeshInstance2D::set_blend_shape_value(int p_blend_shape, float p_value) {
	if (source_mesh.is_null()) {
		return;
	}

	if (blend_shape_tracks.size() != source_mesh->get_blend_shape_count()) {
		blend_shape_tracks.resize(source_mesh->get_blend_shape_count());
	}

	ERR_FAIL_INDEX(p_blend_shape, (int)blend_shape_tracks.size());

	if (Math::is_equal_approx(blend_shape_tracks[p_blend_shape], p_value)) {
		return;
	}

	blend_shape_tracks.write[p_blend_shape] = p_value;

	display_mesh = bake_mesh_from_current_blend_shape_mix(display_mesh);

	updating_mesh = true;
	MeshInstance2D::set_mesh(display_mesh);
	updating_mesh = false;
}

Ref<ArrayMesh> BlendShapeMeshInstance2D::bake_mesh_from_current_blend_shape_mix(Ref<ArrayMesh> p_existing) {
	Ref<ArrayMesh> src_array_mesh = source_mesh;
	ERR_FAIL_COND_V_MSG(src_array_mesh.is_null(), Ref<ArrayMesh>(), "The source mesh must be a valid ArrayMesh.");

	Ref<ArrayMesh> bake_mesh;

	if (p_existing.is_valid()) {
		ERR_FAIL_COND_V_MSG(src_array_mesh == p_existing, Ref<ArrayMesh>(), "The source mesh can not be the same mesh as the existing mesh.");
		bake_mesh = p_existing;
	} else {
		bake_mesh.instantiate();
	}

	Mesh::BlendShapeMode blend_shape_mode = src_array_mesh->get_blend_shape_mode();
	int mesh_surface_count = src_array_mesh->get_surface_count();

	bake_mesh->clear_surfaces();
	bake_mesh->set_blend_shape_mode(blend_shape_mode);

	for (int surface_index = 0; surface_index < mesh_surface_count; surface_index++) {
		uint32_t surface_format = src_array_mesh->surface_get_format(surface_index);
		ERR_CONTINUE(0 == (surface_format & Mesh::ARRAY_FORMAT_VERTEX));

		const Array &source_mesh_arrays = src_array_mesh->surface_get_arrays(surface_index);
		ERR_FAIL_COND_V(source_mesh_arrays.size() != Mesh::ARRAY_MAX, Ref<ArrayMesh>());

		const Vector<Vector2> &source_mesh_vertex_array = source_mesh_arrays[Mesh::ARRAY_VERTEX];

		Array new_mesh_arrays;
		new_mesh_arrays.resize(Mesh::ARRAY_MAX);
		for (int i = 0; i < source_mesh_arrays.size(); i++) {
			if (i == Mesh::ARRAY_VERTEX) {
				continue;
			}
			new_mesh_arrays[i] = source_mesh_arrays[i];
		}

		Vector<Vector2> lerped_vertex_array = source_mesh_vertex_array;
		const Vector2 *source_vertices_ptr = source_mesh_vertex_array.ptr();
		Vector2 *lerped_vertices_ptrw = lerped_vertex_array.ptrw();

		const Array &blendshapes_mesh_arrays = src_array_mesh->surface_get_blend_shape_arrays(surface_index);
		int blend_shape_count = src_array_mesh->get_blend_shape_count();
		ERR_FAIL_COND_V(blendshapes_mesh_arrays.size() != blend_shape_count, Ref<ArrayMesh>());

		for (int blendshape_index = 0; blendshape_index < blend_shape_count; blendshape_index++) {
			float blend_weight = get_blend_shape_value(blendshape_index);
			if (Math::is_zero_approx(blend_weight)) {
				continue;
			}

			const Array &blendshape_mesh_arrays = blendshapes_mesh_arrays[blendshape_index];
			const Vector<Vector2> &blendshape_vertex_array = blendshape_mesh_arrays[Mesh::ARRAY_VERTEX];

			ERR_FAIL_COND_V(source_mesh_vertex_array.size() != blendshape_vertex_array.size(), Ref<ArrayMesh>());
			const Vector2 *blendshape_vertices_ptr = blendshape_vertex_array.ptr();

			if (blend_shape_mode == Mesh::BLEND_SHAPE_MODE_NORMALIZED) {
				for (int i = 0; i < source_mesh_vertex_array.size(); i++) {
					const Vector2 &source_vertex = source_vertices_ptr[i];
					const Vector2 &blendshape_vertex = blendshape_vertices_ptr[i];
					Vector2 lerped_vertex = source_vertex.lerp(blendshape_vertex, blend_weight) - source_vertex;
					lerped_vertices_ptrw[i] += lerped_vertex;
				}
			} else if (blend_shape_mode == Mesh::BLEND_SHAPE_MODE_RELATIVE) {
				for (int i = 0; i < source_mesh_vertex_array.size(); i++) {
					const Vector2 &blendshape_vertex = blendshape_vertices_ptr[i];
					lerped_vertices_ptrw[i] += blendshape_vertex * blend_weight;
				}
			}
		}

		new_mesh_arrays[Mesh::ARRAY_VERTEX] = lerped_vertex_array;
		bake_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, new_mesh_arrays, Array(), Dictionary(), surface_format);
	}

	return bake_mesh;
}

void BlendShapeMeshInstance2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_blend_shape_count"), &BlendShapeMeshInstance2D::get_blend_shape_count);
	ClassDB::bind_method(D_METHOD("find_blend_shape_by_name", "name"), &BlendShapeMeshInstance2D::find_blend_shape_by_name);
	ClassDB::bind_method(D_METHOD("get_blend_shape_value", "blend_shape_idx"), &BlendShapeMeshInstance2D::get_blend_shape_value);
	ClassDB::bind_method(D_METHOD("set_blend_shape_value", "blend_shape_idx", "value"), &BlendShapeMeshInstance2D::set_blend_shape_value);
	ClassDB::bind_method(D_METHOD("bake_mesh_from_current_blend_shape_mix", "existing"), &BlendShapeMeshInstance2D::bake_mesh_from_current_blend_shape_mix, DEFVAL(Ref<ArrayMesh>()));
}