export module clay:wasm;
import gelo;
import hai;
import jute;
import silog;
import sires;
import stubby;
import sv;

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
}
