#pragma leco app

import clay;
import dotz;
import hai;
import vinyl;

struct vtx {
  dotz::vec2 pos;
  dotz::vec2 size;
  dotz::vec4 colour;
  unsigned id;
};
struct pc {
  float aspect;
};

// "das" is a simple pipeline
struct app_stuff : vinyl::base_app_stuff {
  using pipeline_t = clay::das::pipeline<vtx>;
  pipeline_t ppl {{
    .app = this,
    .shader = "poc",
    .texture = "poc",
    .max_instances = 10240,
    .vertex_attributes = pipeline_t::vertex_attributes(
        &vtx::pos,
        &vtx::size,
        &vtx::colour,
        &vtx::id),
    .push_constant = clay::vertex_push_constant<pc>(),
  }};

  app_stuff() : base_app_stuff { "poc" } {}
};
struct ext_stuff {};
using vv = vinyl::v<app_stuff, ext_stuff>;

const int i = [] {
  // TODO: jojo::on_error
  vv::setup([] {
    auto _ = vv::as()->ppl.map();
  });
  return 0;
}();
