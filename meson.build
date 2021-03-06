project(
        'action_binding_layer',
        ['cpp', 'c'],
        version: '0.1',
        license: 'BSL-1.0',
        meson_version: '>=0.49.0',
        default_options: [
                'cpp_std=c++17',
                'warning_level=3',
        ],
)

subdir('external')

openxr_dep = dependency('openxr', required: true)
glfw_dep = dependency('glfw3', required: true)

cc = meson.get_compiler('c')
if host_machine.system() == 'linux'
    dl_dep = cc.find_library('dl', required: false)
else
    dl_dep = dependency('', required: false)
endif

lib = library('XrApiLayer_action_binding',
	'api_layer_action_binding.cpp',
	'ui.cpp',
	'util.cpp',
	dependencies: [openxr_dep, libimgui_core_dep, glfw_dep, glad_dep, dl_dep],
	)

# TODO: use install path
full_path = lib.full_path()

build_dir = meson.current_build_dir()

conf_data = configuration_data()
conf_data.set('library_path', full_path)
conf_file = configure_file(input : 'XrApiLayer_action_binding.json.in',
	output : 'XrApiLayer_action_binding.json',
	configuration : conf_data)

# TODO: cross platform instructions
message('===========================================================')
message('Now copy or symlink the layer manifest to a layer directory')
message('ln -s ' +  join_paths(build_dir, 'XrApiLayer_action_binding.json') + ' ~/.local/share/openxr/1/api_layers/implicit.d/XrApiLayer_action_binding.json')
message('Note: ~/.config/openxr/1/api_layers/implicit.d does not work')
message('===========================================================')
