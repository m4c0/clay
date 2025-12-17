#pragma leco app

import clay;
import dotz;
import hai;
import vinyl;

struct vtx {
  dotz::vec2 pos;
  dotz::vec2 size;
};

// "clay" currently focus on defining certain reusable patterns
struct app_stuff : vinyl::base_app_stuff {
  clay::nearest_texture txt { "poc" };
  clay::vert_shader vert { "poc", [] {} };
  clay::vert_shader frag { "poc", [] {} };
  clay::buffer<vtx> buffer { 10240 };

  app_stuff() : base_app_stuff { "poc" } {}
};
struct ext_stuff {};
using vv = vinyl::v<app_stuff, ext_stuff>;

const int i = [] {
  // TODO: jojo::on_error
  vv::setup([] {
  });
  return 0;
}();
