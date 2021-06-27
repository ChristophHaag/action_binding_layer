# borrowed from mesa/src/imgui/meson.build

glad_inc = include_directories('./include')

libglad = static_library(
  'imgui_core',
  files('src/glad.c'),
  cpp_args : ['-w'],
  install : false,
  include_directories: glad_inc
)

glad_dep = declare_dependency(
  link_with : libglad,
  include_directories : glad_inc
)
