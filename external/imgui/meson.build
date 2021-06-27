# borrowed from mesa/src/imgui/meson.build

libimgui_core = static_library(
  'imgui_core',
  files('imgui.cpp', 'imgui_draw.cpp', 'imgui_widgets.cpp', 'imgui_tables.cpp', 'imgui_impl_glfw.cpp', 'imgui_impl_opengl3.cpp'),
  cpp_args : ['-w', '-DIMGUI_IMPL_OPENGL_LOADER_GLAD'],
  dependencies: [glad_dep],
  install : false,
)

libimgui_core_dep = declare_dependency(
  link_with : libimgui_core,
  include_directories : include_directories('.')
)
