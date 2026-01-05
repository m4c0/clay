export module clay:vulkan;
import dotz;
import hai;
import jute;
import sv;
import vinyl;
import voo;
import wagen;

namespace clay {
  export struct nearest_texture {
    vee::descriptor_set_layout dsl = vee::create_descriptor_set_layout({
      vee::dsl_fragment_sampler(),
    });
    vee::descriptor_pool dpool = vee::create_descriptor_pool(1, {
      vee::combined_image_sampler(1),
    });
    vee::descriptor_set dset = vee::allocate_descriptor_set(*dpool, *dsl);
    vee::sampler smp = [] {
      auto info = vee::sampler_create_info {};
      info.address_mode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
      info.nearest();
      info.unnormalizedCoordinates = wagen::vk_true;
      return vee::create_sampler(info);
    }();
    voo::bound_image img {};

    explicit nearest_texture(sv name) {
      voo::load_image(name, &img, [this](auto) {
        vee::update_descriptor_set(dset, 0, 0, *img.iv, *smp);
      });
    }

    [[nodiscard]] explicit operator bool() const { return true; }
  };

  export struct frag_shader : voo::frag_shader {
    frag_shader(sv name, hai::fn<void> callback) : voo::frag_shader { jute::fmt<"%s.frag.spv">(name) } { callback(); }
  };
  export struct vert_shader : voo::vert_shader {
    vert_shader(sv name, hai::fn<void> callback) : voo::vert_shader { jute::fmt<"%s.vert.spv">(name) } { callback(); }
  };

  export using vertex_attributes_t = hai::view<wagen::VkVertexInputAttributeDescription>;
  export template<typename T> class buffer {
    voo::bound_buffer m_buf;

    unsigned m_count {};
  public:
    explicit buffer(unsigned max) :
      m_buf {
        voo::bound_buffer::create_from_host(
          wagen::physical_device(),
          max * sizeof(T),
          vee::buffer_usage::vertex_buffer)
      }
    {}

    [[nodiscard]] constexpr auto count() const { return m_count; }

    [[nodiscard]] auto map() {
      return voo::memiter<T> { *m_buf.memory, &m_count };
    }

    [[nodiscard]] static auto vertex_input_bind_per_instance() {
      return vee::vertex_input_bind_per_instance(sizeof(T));
    }
    [[nodiscard]] static auto vertex_input_bind() {
      return vee::vertex_input_bind(sizeof(T));
    }

    [[nodiscard]] static auto vertex_attribute(float (T::*m)) {
      return vee::vertex_attribute_float(0, traits::offset_of(m));
    }
    [[nodiscard]] static auto vertex_attribute(dotz::vec2 (T::*m)) {
      return vee::vertex_attribute_vec2(0, traits::offset_of(m));
    }
    [[nodiscard]] static auto vertex_attribute(dotz::vec4 (T::*m)) {
      return vee::vertex_attribute_vec4(0, traits::offset_of(m));
    }
    [[nodiscard]] static auto vertex_attribute(unsigned (T::*m)) {
      return vee::vertex_attribute_uint(0, traits::offset_of(m));
    }
    [[nodiscard]] static auto vertex_attributes(auto &&... attrs) {
      return vertex_attributes_t { vertex_attribute(attrs)...  };
    }

    void bind(vee::command_buffer cb) {
      vee::cmd_bind_vertex_buffers(cb, 0, *m_buf.buffer, 0);
    }
  };

  export using push_constant_t = wagen::VkPushConstantRange;
  export template<typename T> auto vertex_push_constant() {
    return vee::vertex_push_constant_range<T>();
  }
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

    vee::render_pass m_rp;
    vee::pipeline_layout m_pl;
    vee::gr_pipeline m_ppl;

  public:
    pipeline(const params & p) :
      m_txt { p.texture }
    , m_buf { p.max_instances }
    , m_rp { voo::single_att_render_pass(p.app->dq) }
    , m_pl { vee::create_pipeline_layout(*m_txt.dsl, p.push_constant) }
    , m_ppl { vee::create_graphics_pipeline({
      .pipeline_layout = *m_pl,
      .render_pass = *m_rp,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
      .back_face_cull = false,
      .shaders {
        *vert_shader(p.shader, [] {}),
        *frag_shader(p.shader, [] {}),
      },
      .bindings { m_buf.vertex_input_bind_per_instance() },
      .attributes = p.vertex_attributes,
    }) }
    {}

    [[nodiscard]] explicit operator bool() const { return true; }

    auto & texture() { return m_txt; }
    auto map() { return m_buf.map(); }

    void cmd_draw(vee::command_buffer cb, auto * pc) {
      vee::cmd_bind_gr_pipeline(cb, *m_ppl);
      vee::cmd_bind_descriptor_set(cb, *m_pl, 0, m_txt.dset);
      vee::cmd_push_vertex_constants(cb, *m_pl, pc);
      m_buf.bind(cb);
      vee::cmd_draw(cb, 4, m_buf.count());
    }

    static auto vertex_attributes(auto &&... args) {
      return buffer<T>::vertex_attributes(args...);
    }
  };
}
