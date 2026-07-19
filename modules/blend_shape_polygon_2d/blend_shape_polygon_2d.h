#ifndef BLEND_SHAPE_POLYGON2D_H
#define BLEND_SHAPE_POLYGON2D_H

#include "scene/2d/polygon_2d.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "core/object/class_db.h"

class BlendShapePolygon2D : public Polygon2D {
	GDCLASS(BlendShapePolygon2D, Polygon2D);

private:
	struct ShapeData {
		String name;
		PackedVector2Array vertices;
		float weight = 0.0f;
	};

	PackedVector2Array raw_base_polygon;
	Vector<ShapeData> blend_shapes;

	HashMap<StringName, int> value_property_map;
	HashMap<StringName, int> vertices_property_map;

	bool is_dirty = false;
	bool is_updating_mesh = false;

	void _rebuild_property_maps();
	void update_blend();

protected:
	static void _bind_methods();

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	void _notification(int p_what);

public:
	BlendShapePolygon2D();
	~BlendShapePolygon2D();

	void set_blend_shape_value(int p_blend_shape, float p_value);
	float get_blend_shape_value(int p_blend_shape) const;
	int find_blend_shape_by_name(const StringName &p_name) const;

	void add_blend_shape(const String &p_name);
	void remove_blend_shape(const String &p_name);
	int get_blend_shape_count() const;
	String get_blend_shape_name(int p_index) const;

	void set_blend_shape_vertices(const String &p_name, const PackedVector2Array &p_vertices);
	PackedVector2Array get_blend_shape_vertices(const String &p_name) const;

	void set_all_blend_shape_values(float p_value);
};

#endif // BLEND_SHAPE_POLYGON2D_H