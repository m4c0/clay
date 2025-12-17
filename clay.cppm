export module clay;

#ifdef LECO_TARGET_WASM
export import :wasm;
#else
export import :vulkan;
#endif
