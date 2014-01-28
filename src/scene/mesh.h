////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012, 2013
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// raw 3D mesh container
//

namespace octet {
  class mesh : public resource {
    ref<gl_resource> vertices;
    ref<gl_resource> indices;

    // attribute formats
    enum { max_slots = 16 };
    unsigned format[max_slots];

    unsigned num_indices;
    unsigned num_vertices;
    unsigned short stride;
    unsigned short mode;
    unsigned short index_type;
    unsigned short normalized;

    uint8_t num_slots;

    // optional skin
    ref<skin> mesh_skin;
    
    // bounding box
    aabb mesh_aabb;

  public:
    RESOURCE_META(mesh)

    mesh(skin *_skin=0) {
      init(_skin);
    }

    void visit(visitor &v) {
      v.visit(vertices, atom_vertices);
      v.visit(indices, atom_indices);
      v.visit(format, atom_format);
      v.visit(num_indices, atom_num_indices);
      v.visit(num_vertices, atom_num_vertices);
      v.visit(stride, atom_stride);
      v.visit(mode, atom_mode);
      v.visit(index_type, atom_index_type);
      v.visit(normalized, atom_normalized);
      v.visit(num_slots, atom_num_slots);
      v.visit(mesh_skin, atom_mesh_skin);
      v.visit(mesh_aabb, atom_mesh_aabb);
    }

    ~mesh() {
    }

    void init(skin *_skin=0) {
      vertices = new gl_resource();
      indices = new gl_resource();

      memset(format, 0, sizeof(format));

      num_indices = 0;
      num_vertices = 0;
      stride = 0;
      mode = 0;
      index_type = 0;
      normalized = 0;

      num_slots = 0;
      index_type = GL_UNSIGNED_SHORT;
      mode = GL_TRIANGLES;

      mesh_skin = _skin;
    }

    void clear_attributes() {
      num_slots = 0;
    }

    // eg. add_attribute(attribute_pos, 3, GL_FLOAT, 0)
    unsigned add_attribute(unsigned attr, unsigned size, unsigned kind, unsigned offset, unsigned norm=0) {
      assert(num_slots < max_slots);
      format[num_slots] = (offset << 9) + (attr << 5) + ((size-1) << 3) + (kind - GL_BYTE);
      if (norm) normalized |= 1 << num_slots;
      return num_slots++;
    }

    static unsigned kind_size(unsigned kind) {
      static const uint8_t bytes[] = { 1, 1, 2, 2, 4, 4, 4, 4 };
      return kind < GL_BYTE || kind > GL_FLOAT ? 0 : bytes[kind - GL_BYTE];
    }

    unsigned get_offset(unsigned slot) const {
      return ( format[slot] >> 9 ) & 0x3f;
    }

    unsigned get_attr(unsigned slot) const {
      return ( format[slot] >> 5 ) & 0x0f;
    }

    unsigned get_size(unsigned slot) const {
      return ( ( format[slot] >> 3 ) & 0x03 ) + 1;
    }

    unsigned get_kind(unsigned slot) const {
      return ( ( format[slot] >> 0 ) & 0x07 ) + GL_BYTE;
    }

    unsigned get_stride() const {
      return stride;
    }

    unsigned get_num_vertices() const {
      return num_vertices;
    }

    unsigned get_num_indices() const {
      return num_indices;
    }

    unsigned get_mode() const {
      return mode;
    }

    unsigned get_index_type() const {
      return index_type;
    }

    unsigned get_num_slots() const {
      return num_slots;
    }

    // get the optional skin data
    skin *get_skin() const {
      return (skin*)mesh_skin;
    }

    void set_num_vertices(unsigned value) {
      num_vertices = value;
    }

    void set_num_indices(unsigned value) {
      num_indices = value;
    }

    void set_mode(unsigned value) {
      mode = value;
    }

    // set the optional skin
    // note: the mesh state owns the skin.
    void set_skin(skin *value) {
      mesh_skin = value;
    }
    
    // set the axis aligned bounding box of the untransformed mesh
    void set_aabb(const aabb &value) {
      mesh_aabb = value;
    }

    // get the axis aligned bounding box of the untransformed mesh
    aabb get_aabb() {
      return mesh_aabb;
    }

    // return true if this mesh has a particular attribute
    bool has_attribute(unsigned attr) {
      for (unsigned i = 0; i != num_slots; ++i) {
        if (get_attr(i) == attr) {
          return true;
        }
      }
      return false;
    }

    // get which slot a particular attribute is in
    unsigned get_slot(unsigned attr) const {
      for (unsigned i = 0; i != max_slots; ++i) {
        if (!format[i]) break;
        if (get_attr(i) == attr) {
          return i;
        }
      }
      return ~0;
    }

    // get a vec4 value of an attribute (only when not in a vbo)
    vec4 get_value(unsigned slot, unsigned index) const {
      if (get_kind(slot) == GL_FLOAT) {
        const float *src = (float*)((uint8_t*)vertices->lock_read_only() + stride * index + get_offset(slot));
        unsigned size = get_size(slot);
        float x = src[0];
        float y = size > 1 ? src[1] : 0;
        float z = size > 2 ? src[2] : 0;
        float w = size > 3 ? src[3] : 1;
        vertices->unlock_read_only();
        return vec4(x, y, z, w);
	  } else if (get_kind(slot) == GL_UNSIGNED_BYTE) {
        const uint8_t *src = (uint8_t*)((uint8_t*)vertices->lock_read_only() + stride * index + get_offset(slot));
        unsigned size = get_size(slot);
        float x = src[0]*(1.0f/255);
        float y = size > 1 ? src[1]*(1.0f/255) : 0;
        float z = size > 2 ? src[2]*(1.0f/255) : 0;
        float w = size > 3 ? src[3]*(1.0f/255) : 1;
        vertices->unlock_read_only();
        return vec4(x, y, z, w);
      }
      return vec4(0, 0, 0, 0);
    }

    void get_values(unsigned slot, uint8_t *dest, unsigned dest_stride) {
      unsigned bytes = kind_size(get_kind(slot)) * get_size(slot);
      unsigned nv = get_num_vertices();
      uint8_t *src = (uint8_t*)vertices->lock_read_only() + get_offset(slot);
      for (unsigned i  = 0; i != nv; ++i) {
        memcpy(dest, src, bytes);
        src += stride;
        dest += dest_stride;
      }
      vertices->unlock_read_only();
    }

    // set a vec4 value of an attribute (only when not in a vbo)
    void set_value(unsigned slot, unsigned index, vec4 value) {
      if (get_kind(slot) == GL_FLOAT) {
        float *src = (float*)((uint8_t*)vertices->lock() + stride * index + get_offset(slot));
        unsigned size = get_size(slot);
        src[0] = value[0];
        if (size > 1) src[1] = value[1];
        if (size > 2) src[2] = value[2];
        if (size > 3) src[3] = value[3];
        vertices->unlock();
  	  } else if (get_kind(slot) == GL_UNSIGNED_BYTE) {
        uint8_t *src = (uint8_t*)((uint8_t*)vertices->lock() + stride * index + get_offset(slot));
        unsigned size = get_size(slot);
        src[0] = (uint8_t)( value[0] * 255.0f );
        if (size > 1) src[1] = (uint8_t)( value[1] * 255.0f );
        if (size > 2) src[2] = (uint8_t)( value[2] * 255.0f );
        if (size > 3) src[3] = (uint8_t)( value[3] * 255.0f );
        vertices->unlock();
      }
    }

    unsigned get_index(unsigned index) const {
      unsigned result = 0;
      if (index_type == GL_UNSIGNED_SHORT) {
        unsigned short *src = (unsigned short*)((uint8_t*)indices->lock_read_only() + index*2);
        result = *src;
        indices->unlock_read_only();
      } else if (index_type == GL_UNSIGNED_INT) {
        unsigned int *src = (unsigned int*)((uint8_t*)indices->lock_read_only() + index*4);
        result = *src;
        indices->unlock_read_only();
      }
      return result;
    }

    void allocate(unsigned vsize, unsigned isize) {
      vertices->allocate(GL_ARRAY_BUFFER, vsize);
      indices->allocate(GL_ELEMENT_ARRAY_BUFFER, isize);
    }

    void assign(unsigned vsize, unsigned isize, uint8_t *vsrc, uint8_t *isrc) {
      vertices->assign(vsrc, 0, vsize);
      indices->assign(isrc, 0, isize);
    }

    void set_params(unsigned stride_, unsigned num_indices_, unsigned num_vertices_, unsigned mode_, unsigned index_type_) {
      stride = stride_;
      num_indices = num_indices_;
      num_vertices = num_vertices_;
      mode = mode_;
      index_type = index_type_;
    }

    void dump(FILE *file) {
      fprintf(file, "<model mode=%04x index_type=%04x stride=%d>\n", mode, index_type, stride);
      for (unsigned slot = 0; slot != num_slots; ++slot) {
        fprintf(file, "  <slot n=%d attr=%d kind=%04x size=%d offset=%d>\n", slot, get_attr(slot), get_kind(slot), get_size(slot), get_offset(slot)); 
        const char *fmt[] = { "", "    [%d %f]\n", "    [%d %f %f]\n", "    [%d %f %f %f]\n", "    [%d %f %f %f %f]\n" };
        for (unsigned i = 0; i != num_vertices; ++i) {
          vec4 value = get_value(slot, i);
          fprintf(file, fmt[get_size(slot)], i, value[0], value[1], value[2], value[3]);
        }
        fprintf(file, "  </slot>\n");
      }
      fprintf(file, "  <indices>\n    ");
      unsigned ni = get_num_indices();
      for (unsigned i = 0; i != ni; ++i) {
        fprintf(file, "%d ", get_index(i));
      }
      fprintf(file, "\n  </indices>\n");
      fprintf(file, "</model>\n");
    }

    // render a mesh with OpenGL
    // assume the shader, uniforms and render params are already set up.
    void enable_attributes() const {
      vertices->bind();

      unsigned n = normalized;
      for (unsigned slot = 0; slot != get_num_slots(); ++slot) {
        unsigned size = get_size(slot);
        unsigned kind = get_kind(slot);
        unsigned attr = get_attr(slot);
        unsigned offset = get_offset(slot);
        glVertexAttribPointer(attr, size, kind, n & 1, get_stride(), (void*)(offset));
        glEnableVertexAttribArray(attr);
        n >>= 1;
      }
    }

    void draw() {
      indices->bind();
      glDrawElements(get_mode(), get_num_indices(), get_index_type(), (GLvoid*)0);
    }

    void disable_attributes() {
      for (unsigned slot = 0; slot != get_num_slots(); ++slot) {
        unsigned attr = get_attr(slot);
        glDisableVertexAttribArray(attr);
      }
    }

    void render() {
      enable_attributes();
      draw();
      disable_attributes();
    }

    void make_cube(float size = 1.0f) {
      init();
      mesh_builder b;
    
      b.init(6*4, 6*6);

      b.add_cube(size);
      b.get_mesh(*this);
    }

    void make_aa_box(float x, float y, float z) {
      init();
      mesh_builder b;
    
      b.init(6*4, 6*6);
      b.scale(x, y, z);

      b.add_cube(1.0f);
      b.get_mesh(*this);
    }

    void make_plane(float size = 1.0f, unsigned nx=1, unsigned ny=1) {
      init();
      mesh_builder b;
      b.init(nx*ny*2, nx*ny*6);
      b.add_plane(size, nx, ny);
      b.get_mesh(*this);
    }

    void make_sphere(float radius=1.0f, unsigned slices=16, unsigned stacks=16) {
      init();
      mesh_builder b;
    
      b.init(stacks*(slices+1), stacks*slices*6);

      b.add_sphere(radius, slices, stacks, 8);
      b.get_mesh(*this);
    }

    void make_cone(float radius=1.0f, float height=1.0f, unsigned slices=32, unsigned stacks=1) {
      init();
      mesh_builder b;
    
      b.init(stacks*slices, stacks*slices*6+1);

      b.add_cone(radius, height, slices, stacks);
      b.get_mesh(*this);
    }

    // make a bunch of lines to show normals.
    // render this with GL_LINES
    void make_normal_visualizer(const mesh &source, float length, unsigned normal_attr) {
      init();
      mesh_builder builder;
    
      builder.init(source.get_num_vertices()*2, source.get_num_vertices()*2);

      unsigned pos_slot = source.get_slot(attribute_pos);
      unsigned normal_slot = source.get_slot(normal_attr);
      for (unsigned i = 0; i != source.get_num_vertices(); ++i) {
        vec4 pos = source.get_value(pos_slot, i);
        vec4 normal = source.get_value(normal_slot, i);
        unsigned idx = builder.add_vertex(pos, normal, 0, 0);
        builder.add_vertex(pos + normal * length, normal, 0, 0);
        builder.add_index(idx);
        builder.add_index(idx+1);
      }

      builder.get_mesh(*this);
      set_mode( GL_LINES );
    }

    // make the normal, tangent, bitangent space for each vertex
    void add_3d_normals(const mesh &source) {
      init();
      if (source.get_mode() != GL_TRIANGLES || source.get_num_indices() % 3 != 0) {
        printf("warning: make_3d_normals expected triangles\n");
        return;
      }

      //s = source.s; 

      // add tangent and bitangent
      unsigned stride = source.get_stride();
      unsigned tangent_slot = add_attribute(attribute_tangent, 3, GL_FLOAT, stride);
      unsigned bitangent_slot = add_attribute(attribute_bitangent, 3, GL_FLOAT, stride + 12);

      unsigned vsize = (stride + 24) * source.get_num_vertices();
      unsigned isize = source.get_indices()->get_size(); //mesh::kind_size(get_index_type()) * source.get_num_indices();
      allocate(vsize, isize);
      set_params(stride + 24, source.get_num_indices(), source.get_num_vertices(), source.get_mode(), source.get_index_type());
      indices = source.get_indices();

      unsigned pos_slot = source.get_slot(attribute_pos);
      unsigned uv_slot = source.get_slot(attribute_uv);
      //unsigned normal_slot = source.get_slot(attribute_normal);

      for (unsigned slot = 0; slot != source.get_num_slots(); ++slot) {
        for (unsigned i = 0; i != source.get_num_vertices(); ++i) {
          vec4 value = source.get_value(slot, i);
          set_value(slot, i, value);
        }
      }

      for (unsigned i = 0; i != source.get_num_vertices(); ++i) {
        vec4 value(0, 0, 0, 0);
        set_value(tangent_slot, i, value);
        set_value(bitangent_slot, i, value);
      }

      for (unsigned i = 0; i != source.get_num_indices(); i += 3) {
        unsigned idx[3] = {
          source.get_index(i),
          source.get_index(i+1),
          source.get_index(i+2)
        };
        vec4 uv0 = source.get_value(uv_slot, idx[0]);
        vec4 uv1 = source.get_value(uv_slot, idx[1]);
        vec4 uv2 = source.get_value(uv_slot, idx[2]);
        vec4 pos0 = source.get_value(pos_slot, idx[0]);
        vec4 pos1 = source.get_value(pos_slot, idx[1]);
        vec4 pos2 = source.get_value(pos_slot, idx[2]);
        vec4 duv1 = uv1 - uv0;
        vec4 duv2 = uv2 - uv0;
        vec4 dpos1 = pos1 - pos0;
        vec4 dpos2 = pos2 - pos0;

        // solve:
        //   duv1.x() * tangent + duv1.y() * bitangent = dpos1
        //   duv2.x() * tangent + duv2.y() * bitangent = dpos2

        //float rdet = 1.0f / (duv1.x() * duv2.y() - duv1.y() * duv2.x());
        float rdet = 1.0f;
        vec4 tangent = (dpos1 * duv2.y() - dpos2 * duv1.y()) * rdet;
        vec4 bitangent = (dpos2 * duv1.x() - dpos1 * duv2.x()) * rdet;

        /*vec4 normal = source.get_value(normal_slot, idx[0]);
        printf("normal=%s\n", normal.toString());
        printf("tangent=%s\n", tangent.toString());
        printf("bitangent=%s\n", bitangent.toString());
        printf("duv1.x() * tangent + duv1.y() * bitangent=%s\n", (tangent * duv1.x() + bitangent * duv1.y()).toString());
        printf("dpos1=%s\n", dpos1.toString());
        printf("duv2.x() * tangent + duv2.y() * bitangent=%s\n", (tangent * duv2.x() + bitangent * duv2.y()).toString());
        printf("dpos2=%s\n", dpos2.toString());*/

        for (unsigned j = 0; j != 3; ++j) {
          vec4 new_tangent = tangent + get_value(tangent_slot, idx[j]);
          vec4 new_bitangent = bitangent + get_value(bitangent_slot, idx[j]);
          set_value(tangent_slot, idx[j], new_tangent);
          set_value(bitangent_slot, idx[j], new_bitangent);
        }
      }

      // normalize
      for (unsigned i = 0; i != source.get_num_vertices(); ++i) {
        vec3 new_tangent = get_value(tangent_slot, i).xyz().normalize();
        vec3 new_bitangent = get_value(bitangent_slot, i).xyz().normalize();
        set_value(tangent_slot, i, vec4(new_tangent,0));
        set_value(bitangent_slot, i, vec4(new_bitangent,0));
      }
    }

    // apply a matrix to every position
    void transform(unsigned attr, mat4t &matrix) {
      if (get_num_vertices() == 0) {
        return;
      }

      unsigned slot = get_slot(attr);
      for (unsigned i = 0; i != get_num_vertices(); ++i) {
        vec4 pos = get_value(slot, i);
        vec4 tpos = pos * matrix;
        set_value(slot, i, tpos);
      }
    }

    // *very* slow ray cast.
    // returns "barycentric" coordinates.
    // eg. hit pos = bary[0] * pos0 + bary[1] * pos1 + bary[2] * pos2;
    // eg. hit uv = bary[0] * uv0 + bary[1] * uv1 + bary[2] * uv2;
    bool ray_cast(const vec4 &org, const vec4 &dir, int indices[], vec4 &bary) {
      //static FILE *file = fopen("c:/tmp/3.txt","w");
      //fprintf(file, "org=%s dir=%s\n", org.toString(), dir.toString());
      unsigned pos_slot = get_slot(attribute_pos);
      for (unsigned i = 0; i != get_num_indices(); i += 3) {
        unsigned idx[3] = {
          get_index(i),
          get_index(i+1),
          get_index(i+2)
        };

        vec4 a = get_value(pos_slot, idx[0]) - org;
        vec4 b = get_value(pos_slot, idx[1]) - org;
        vec4 c = get_value(pos_slot, idx[2]) - org;
        vec4 d = dir;

        //fprintf(file, "a=%s b=%s c=%s d=%s\n", a.toString(), b.toString(), c.toString(), d.toString());

        // solve [ba, bb, bc, bd] * [[ax, ay, az, 1], [bx, by, bz, 1], [cx, cy, cz, 1], [dx, dy, dz, 0]] = [0, 0, 0, 1]
        float det = (
          -a.x()*b.y()*d.z()+a.x()*b.z()*d.y()
          +a.x()*c.y()*d.z()-a.x()*c.z()*d.y()
          +a.y()*b.x()*d.z()-a.y()*b.z()*d.x()
          -a.y()*c.x()*d.z()+a.y()*c.z()*d.x()
          -a.z()*b.x()*d.y()+a.z()*b.y()*d.x()
          +a.z()*c.x()*d.y()-a.z()*c.y()*d.x()
          -b.x()*c.y()*d.z()+b.x()*c.z()*d.y()
          +b.y()*c.x()*d.z()-b.y()*c.z()*d.x()
          -b.z()*c.x()*d.y()+b.z()*c.y()*d.x()
        );

        vec4 bary1 = vec4(
          -b.x()*c.y()*d.z()+b.x()*c.z()*d.y()+b.y()*c.x()*d.z()-b.y()*c.z()*d.x()-b.z()*c.x()*d.y()+b.z()*c.y()*d.x(),
          a.x()*c.y()*d.z()-a.x()*c.z()*d.y()-a.y()*c.x()*d.z()+a.y()*c.z()*d.x()+a.z()*c.x()*d.y()-a.z()*c.y()*d.x(),
          -a.x()*b.y()*d.z()+a.x()*b.z()*d.y()+a.y()*b.x()*d.z()-a.y()*b.z()*d.x()-a.z()*b.x()*d.y()+a.z()*b.y()*d.x(),
          a.x()*b.y()*c.z()-a.x()*b.z()*c.y()-a.y()*b.x()*c.z()+a.y()*b.z()*c.x()+a.z()*b.x()*c.y()-a.z()*b.y()*c.x()
        );

        vec4 bary2 = bary1 * det;

        if (bary2.x() >= 0 && bary2.y() >= 0 && bary2.z() >= 0 && bary2.w() >= 0) {
          //fprintf(file, "!! %d %d %d  %s\n", idx[0], idx[1], idx[2], (bary1/det).toString());
          indices[0] = idx[0];
          indices[1] = idx[1];
          indices[2] = idx[2];
          bary = bary1 / det;
          return true;
        }
      }
      //fclose(file);
      //exit(1);
      return false;
    }

    // access the vbo or memory buffer
    gl_resource *get_vertices() const {
      return vertices;
    }

    // access the index buffer or memory buffer
    gl_resource *get_indices() const {
      return indices;
    }

    // access the vbo or memory buffer
    void set_vertices(gl_resource *value) {
      vertices = value;
    }

    // access the index buffer or memory buffer
    void set_indices(gl_resource *value) {
      indices = value;
    }

    void get_edges(hash_map<uint64_t, unsigned> &edges) {
    }
  };
}
