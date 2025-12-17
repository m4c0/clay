export module clay:vulkan;
import hai;
import jute;
import sv;
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

    void bind(vee::command_buffer cb) {
      vee::cmd_bind_vertex_buffers(cb, 0, *m_buf.buffer, 0);
    }
  };
}
