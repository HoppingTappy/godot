#include "blend_shape_polygon_2d.h"
#include "core/config/engine.h"
#include <cmath>
#include <cstring>

BlendShapePolygon2D::BlendShapePolygon2D() {}
BlendShapePolygon2D::~BlendShapePolygon2D() {}

void BlendShapePolygon2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_blend_shape_value", "blend_shape", "value"), &BlendShapePolygon2D::set_blend_shape_value);
	ClassDB::bind_method(D_METHOD("get_blend_shape_value", "blend_shape"), &BlendShapePolygon2D::get_blend_shape_value);
	ClassDB::bind_method(D_METHOD("find_blend_shape_by_name", "name"), &BlendShapePolygon2D::find_blend_shape_by_name);

	ClassDB::bind_method(D_METHOD("add_blend_shape", "name"), &BlendShapePolygon2D::add_blend_shape);
	ClassDB::bind_method(D_METHOD("remove_blend_shape", "name"), &BlendShapePolygon2D::remove_blend_shape);
	ClassDB::bind_method(D_METHOD("get_blend_shape_count"), &BlendShapePolygon2D::get_blend_shape_count);
	ClassDB::bind_method(D_METHOD("get_blend_shape_name", "index"), &BlendShapePolygon2D::get_blend_shape_name);

	ClassDB::bind_method(D_METHOD("set_blend_shape_vertices", "name", "vertices"), &BlendShapePolygon2D::set_blend_shape_vertices);
	ClassDB::bind_method(D_METHOD("get_blend_shape_vertices", "name"), &BlendShapePolygon2D::get_blend_shape_vertices);
	ClassDB::bind_method(D_METHOD("set_all_blend_shape_values", "value"), &BlendShapePolygon2D::set_all_blend_shape_values);
}

int BlendShapePolygon2D::find_blend_shape_by_name(const StringName &p_name) const {
	String name_str = p_name;
	for (int i = 0; i < blend_shapes.size(); ++i) {
		if (blend_shapes[i].name == name_str) {
			return i;
		}
	}
	return -1;
}

void BlendShapePolygon2D::_rebuild_property_maps() {
	value_property_map.clear();
	vertices_property_map.clear();

	for (int i = 0; i < blend_shapes.size(); ++i) {
		String base_path = "blend_shapes/" + blend_shapes[i].name;
		value_property_map[base_path] = i;
		vertices_property_map[base_path + "/vertices"] = i;
	}
}

void BlendShapePolygon2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			set_process(true);
			if (raw_base_polygon.size() == 0) {
				raw_base_polygon = get_polygon();
			}
			_rebuild_property_maps();
		} break;

		case NOTIFICATION_PROCESS: {
			if (is_dirty) {
				update_blend();
				is_dirty = false;
			}
		} break;

		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED:
		case NOTIFICATION_DRAW: {
			if (Engine::get_singleton()->is_editor_hint()) {
				bool is_currently_blended = false;
				for (int i = 0; i < blend_shapes.size(); ++i) {
					if (std::abs(blend_shapes[i].weight) > 0.0001f) {
						is_currently_blended = true;
						break;
					}
				}

				if (!is_updating_mesh && !is_currently_blended) {
					PackedVector2Array current_polygon = get_polygon();
					if (current_polygon.size() != raw_base_polygon.size()) {
						for (int i = 0; i < blend_shapes.size(); ++i) {
							blend_shapes.write[i].vertices.resize(current_polygon.size());
						}
						notify_property_list_changed();
					}
					raw_base_polygon = current_polygon;
				}
			}
		} break;
	}
}

bool BlendShapePolygon2D::_set(const StringName &p_name, const Variant &p_value) {
	if (p_name == "blend_shapes/internal/base_polygon") {
		raw_base_polygon = p_value;
		is_dirty = true;
		return true;
	}

	HashMap<StringName, int>::Iterator E_val = value_property_map.find(p_name);
	if (E_val) {
		set_blend_shape_value(E_val->value, p_value);
		return true;
	}

	HashMap<StringName, int>::Iterator E_vtx = vertices_property_map.find(p_name);
	if (E_vtx) {
		blend_shapes.write[E_vtx->value].vertices = p_value;
		is_dirty = true;
		return true;
	}

	String name = p_name;
	if (name.begins_with("blend_shapes/")) {
		String sub_path = name.replace("blend_shapes/", "");

		if (sub_path.ends_with("/vertices")) {
			String shape_name = sub_path.replace("/vertices", "");
			int idx = find_blend_shape_by_name(shape_name);
			if (idx == -1) {
				add_blend_shape(shape_name);
				idx = find_blend_shape_by_name(shape_name);
			}
			if (idx != -1) {
				blend_shapes.write[idx].vertices = p_value;
				is_dirty = true;
				return true;
			}
		}
		else if (!sub_path.contains("/")) {
			int idx = find_blend_shape_by_name(sub_path);
			if (idx == -1) {
				add_blend_shape(sub_path);
				idx = find_blend_shape_by_name(sub_path);
			}
			if (idx != -1) {
				set_blend_shape_value(idx, p_value);
				return true;
			}
		}
	}

	return false;
}

bool BlendShapePolygon2D::_get(const StringName &p_name, Variant &r_ret) const {
	if (p_name == "blend_shapes/internal/base_polygon") {
		r_ret = raw_base_polygon;
		return true;
	}

	HashMap<StringName, int>::ConstIterator E_val = value_property_map.find(p_name);
	if (E_val) {
		r_ret = get_blend_shape_value(E_val->value);
		return true;
	}

	HashMap<StringName, int>::ConstIterator E_vtx = vertices_property_map.find(p_name);
	if (E_vtx) {
		r_ret = blend_shapes[E_vtx->value].vertices;
		return true;
	}

	return false;
}

void BlendShapePolygon2D::_get_property_list(List<PropertyInfo> *p_list) const {
	p_list->push_back(PropertyInfo(
		Variant::PACKED_VECTOR2_ARRAY,
		"blend_shapes/internal/base_polygon",
		PROPERTY_HINT_NONE,
		"",
		PROPERTY_USAGE_STORAGE
	));

	for (int i = 0; i < blend_shapes.size(); ++i) {
		const String &shape_name = blend_shapes[i].name;
		p_list->push_back(PropertyInfo(
			Variant::FLOAT,
			"blend_shapes/" + shape_name,
			PROPERTY_HINT_RANGE,
			"-1.0,1.0,0.01"
		));
		p_list->push_back(PropertyInfo(
			Variant::PACKED_VECTOR2_ARRAY,
			"blend_shapes/" + shape_name + "/vertices",
			PROPERTY_HINT_NONE,
			"",
			PROPERTY_USAGE_STORAGE
		));
	}
}

void BlendShapePolygon2D::add_blend_shape(const String &p_name) {
	if (p_name.is_empty() || find_blend_shape_by_name(p_name) != -1) {
		return;
	}

	ShapeData new_shape;
	new_shape.name = p_name;
	new_shape.vertices.resize(raw_base_polygon.size());
	new_shape.vertices.fill(Vector2());
	new_shape.weight = 0.0f;

	blend_shapes.push_back(new_shape);

	_rebuild_property_maps();
	is_dirty = true;
	notify_property_list_changed();
}

void BlendShapePolygon2D::remove_blend_shape(const String &p_name) {
	int idx = find_blend_shape_by_name(p_name);
	if (idx != -1) {
		blend_shapes.remove_at(idx);
		_rebuild_property_maps();
		is_dirty = true;
		notify_property_list_changed();
	}
}

void BlendShapePolygon2D::update_blend() {
	int64_t base_size = raw_base_polygon.size();
	if (base_size == 0) {
		raw_base_polygon = get_polygon();
		base_size = raw_base_polygon.size();
		if (base_size == 0) return;
	}

	if (Engine::get_singleton()->is_editor_hint()) {
		if (raw_base_polygon.size() != get_polygon().size() && !is_updating_mesh) {
			raw_base_polygon = get_polygon();
			base_size = raw_base_polygon.size();
		}
	}

	PackedVector2Array blended_vertices;
	blended_vertices.resize(base_size);

	const Vector2 *r_base = raw_base_polygon.ptr();
	Vector2 *w_blended = blended_vertices.ptrw();

	std::memcpy(w_blended, r_base, base_size * sizeof(Vector2));

	const int num_shapes = blend_shapes.size();
	for (int s = 0; s < num_shapes; ++s) {
		const ShapeData &shape = blend_shapes[s];

		if (std::abs(shape.weight) <= 0.0001f) {
			continue;
		}

		if (shape.vertices.size() != base_size) {
			continue;
		}

		const Vector2 *r_offset = shape.vertices.ptr();
		const float weight = shape.weight;

		for (int64_t i = 0; i < base_size; ++i) {
			w_blended[i] += r_offset[i] * weight;
		}
	}

	is_updating_mesh = true;
	set_polygon(blended_vertices);
	is_updating_mesh = false;
}

void BlendShapePolygon2D::set_blend_shape_vertices(const String &p_name, const PackedVector2Array &p_vertices) {
	int idx = find_blend_shape_by_name(p_name);
	if (idx != -1) {
		blend_shapes.write[idx].vertices = p_vertices;
		is_dirty = true;
	}
}

PackedVector2Array BlendShapePolygon2D::get_blend_shape_vertices(const String &p_name) const {
	int idx = find_blend_shape_by_name(p_name);
	if (idx != -1) {
		return blend_shapes[idx].vertices;
	}
	return PackedVector2Array();
}

void BlendShapePolygon2D::set_blend_shape_value(int p_blend_shape, float p_value) {
	if (p_blend_shape >= 0 && p_blend_shape < blend_shapes.size()) {
		if (blend_shapes[p_blend_shape].weight != p_value) {
			blend_shapes.write[p_blend_shape].weight = p_value;
			is_dirty = true;
		}
	}
}

float BlendShapePolygon2D::get_blend_shape_value(int p_blend_shape) const {
	if (p_blend_shape >= 0 && p_blend_shape < blend_shapes.size()) {
		return blend_shapes[p_blend_shape].weight;
	}
	return 0.0f;
}

String BlendShapePolygon2D::get_blend_shape_name(int p_index) const {
	if (p_index >= 0 && p_index < blend_shapes.size()) {
		return blend_shapes[p_index].name;
	}
	return "";
}

int BlendShapePolygon2D::get_blend_shape_count() const {
	return blend_shapes.size();
}

void BlendShapePolygon2D::set_all_blend_shape_values(float p_value) {
	bool changed = false;
	for (int i = 0; i < blend_shapes.size(); ++i) {
		if (blend_shapes[i].weight != p_value) {
			blend_shapes.write[i].weight = p_value;
			changed = true;
		}
	}
	if (changed) {
		is_dirty = true;
	}
}