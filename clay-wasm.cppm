export module clay:wasm;
import dotz;
import gelo;
import hai;
import jute;
import silog;
import sires;
import stubby;
import sv;
import vinyl;

namespace clay {
  export struct nearest_texture {
    int id = gelo::create_texture();

    explicit nearest_texture(sv name) {
      sires::read(name, nullptr, [this,name](auto, hai::cstr & bits) {
        using namespace gelo;

        silog::log(silog::info, "[%*s] loaded", static_cast<int>(name.size()), name.begin());

        auto img = stbi::load(bits);
        auto & [ w, h, ch, data ] = img;
        active_texture(TEXTURE0);
        bind_texture(TEXTURE_2D, id);
        tex_image_2d(TEXTURE_2D, 0, RGBA, w, h, 0, RGBA, UNSIGNED_BYTE, *data, w * h * 4);
        tex_parameter_i(TEXTURE_2D, TEXTURE_WRAP_S, CLAMP_TO_EDGE);
        tex_parameter_i(TEXTURE_2D, TEXTURE_WRAP_T, CLAMP_TO_EDGE);
        tex_parameter_i(TEXTURE_2D, TEXTURE_MIN_FILTER, NEAREST);
        tex_parameter_i(TEXTURE_2D, TEXTURE_MAG_FILTER, NEAREST);
      });
    }

    [[nodiscard]] explicit operator bool() const { return id != 0; }
  };

  class shader {
    int m_id = 0;

  protected:
    shader() = default;
    shader(unsigned type, sv name, sv ext, hai::fn<void> callback) {
      auto filename = jute::fmt<"%s.%s.gles">(name, ext);
      sires::read(filename, nullptr, [=,this](auto, hai::cstr & gles) mutable {
        using namespace gelo;

        auto v = gelo::create_shader(type);
        shader_source(v, gles.begin(), gles.size());
        compile_shader(v);
        if (!get_shader_parameter_b(v, COMPILE_STATUS)) {
          char buf[1024] {};
          get_shader_info_log(v, buf, sizeof(buf) - 1);
          silog::log(silog::error, "Error compiling shader:\n%s", buf);
        }
        m_id = v;
        callback();
      });
    }

  public:
    constexpr auto id() const { return m_id; }
    explicit constexpr operator bool() const { return m_id; }
  };
  export struct vert_shader : shader {
    vert_shader() = default;
    explicit vert_shader(sv name, hai::fn<void> callback) : shader { gelo::VERTEX_SHADER, name, "vert", callback } {}
  };
  export struct frag_shader : shader {
    frag_shader() = default;
    explicit frag_shader(sv name, hai::fn<void> callback) : shader { gelo::FRAGMENT_SHADER, name, "frag", callback } {}
  };

  export template<typename T> class mapper {
    hai::varray<T> m_data;
    unsigned * m_count;

  public:
    mapper(unsigned capacity, unsigned * count) : m_data { capacity }, m_count { count } {}
    ~mapper() {
      *m_count = m_data.size();
      gelo::buffer_data(gelo::ARRAY_BUFFER, m_data.begin(), *m_count * sizeof(T), gelo::DYNAMIC_DRAW);
    }

    void operator+=(T t) {
      m_data.push_back(t);
    }
  };

  using vertex_attribute_t = hai::fn<void, unsigned>;
  export using vertex_attributes_t = hai::view<vertex_attribute_t>;
  export template<typename T> class buffer {
    int m_id;
    unsigned m_capacity;
    unsigned m_count;

  public:
    explicit buffer(unsigned capacity) :
      m_id { gelo::create_buffer() }
    , m_capacity { capacity }
    {}

    [[nodiscard]] constexpr auto count() const { return m_count; }

    void bind() {
      gelo::bind_buffer(gelo::ARRAY_BUFFER, m_id);
    }

    [[nodiscard]] auto map() {
      bind();
      return mapper<T> { m_capacity, &m_count };
    }

    [[nodiscard]] static vertex_attribute_t vertex_attribute(float (T::*m)) {
      return [m](unsigned i) {
        using namespace gelo;
        enable_vertex_attrib_array(i);
        vertex_attrib_pointer(i, 1, FLOAT, false, sizeof(T), traits::offset_of(m));
        vertex_attrib_divisor(i, 1);
      };
    }
    [[nodiscard]] static vertex_attribute_t vertex_attribute(dotz::vec2 (T::*m)) {
      return [m](unsigned i) {
        using namespace gelo;
        enable_vertex_attrib_array(i);
        vertex_attrib_pointer(i, 2, FLOAT, false, sizeof(T), traits::offset_of(m));
        vertex_attrib_divisor(i, 1);
      };
    }
    [[nodiscard]] static vertex_attribute_t vertex_attribute(dotz::vec4 (T::*m)) {
      return [m](unsigned i) {
        using namespace gelo;
        enable_vertex_attrib_array(i);
        vertex_attrib_pointer(i, 4, FLOAT, false, sizeof(T), traits::offset_of(m));
        vertex_attrib_divisor(i, 1);
      };
    }
    [[nodiscard]] static vertex_attribute_t vertex_attribute(unsigned (T::*m)) {
      return [m](unsigned i) {
        using namespace gelo;
        enable_vertex_attrib_array(i);
        vertex_attrib_i_pointer(i, 1, UNSIGNED_INT, sizeof(T), traits::offset_of(m));
        vertex_attrib_divisor(i, 1);
      };
    }
    [[nodiscard]] static auto vertex_attributes(auto &&... attrs) {
      return vertex_attributes_t { vertex_attribute(attrs)...  };
    }
  };

  export using push_constant_t = void *;
  export template<typename T> auto vertex_push_constant() { return nullptr; }
}

namespace clay::das {
  export struct params {
    vinyl::base_app_stuff * app;
    sv shader;
    sv texture;
    unsigned max_instances;
    vertex_attributes_t vertex_attributes;
    push_constant_t push_constant;
  };

  export template<typename T> class pipeline {
    nearest_texture m_txt;
    buffer<T> m_buf;

    vert_shader m_vert;
    frag_shader m_frag;
    int m_program = 0;

    void link() {
      if (!m_vert || !m_frag) return;

      using namespace gelo;

      auto p = create_program();
      attach_shader(p, m_vert.id());
      attach_shader(p, m_frag.id());

      link_program(p);
      if (!get_program_parameter_b(p, LINK_STATUS)) {
        char buf[1024] {};
        get_program_info_log(p, buf, sizeof(buf) - 1);
        silog::log(silog::error, "Error linking program:\n%s", buf);
      }

      use_program(p);

      enable(BLEND);
      blend_func(ONE, ONE_MINUS_SRC_ALPHA);

      m_program = p;
    }

  public:
    pipeline(const params & p) :
      m_txt { p.texture }
    , m_buf { p.max_instances }
    , m_vert { p.shader, [this] { link(); } }
    , m_frag { p.shader, [this] { link(); } }
    {
      m_buf.bind();

      auto attrs = p.vertex_attributes;
      for (auto i = 0; i < attrs.size(); i++) attrs[i](i);
    }

    [[nodiscard]] explicit operator bool() const { return m_program != 0; }

    auto map() { return m_buf.map(); }

    auto program() { return m_program; }

    void cmd_draw() {
      using namespace gelo;

      draw_arrays_instanced(TRIANGLE_STRIP, 0, 4, m_buf.count());
    }

    static auto vertex_attributes(auto &&... args) {
      return buffer<T>::vertex_attributes(args...);
    }
  };
}
