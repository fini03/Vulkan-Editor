project(
  'vulkan-tutorial', 'cpp',
  version: '0.1',
  default_options: [
    'cpp_std=c++20',
    'buildtype=release',
    'optimization=3',
  ]
)

# Dependencies
sdl3 = dependency('sdl3')
vulkan = dependency('vulkan')
glm = dependency('glm')

# ImGui Files
imgui_files = files(
  'imgui/imgui.cpp',
  'imgui/imgui_draw.cpp',
  'imgui/imgui_demo.cpp',
  'imgui/imgui_widgets.cpp',
  'imgui/imgui_tables.cpp',
  'imgui/backends/imgui_impl_sdl3.cpp',
  'imgui/backends/imgui_impl_vulkan.cpp',
  'imgui-node-editor/imgui_node_editor.cpp',
  'imgui-node-editor/imgui_node_editor_api.cpp',
  'imgui-node-editor/crude_json.cpp',
  'imgui-node-editor/imgui_canvas.cpp'
)

vulkan_files = files(
	'vulkan_base/vulkan_device.cpp',
)

editor_files = files(
	'vulkan_editor/header.cpp',
	'vulkan_editor/vulkan_view.cpp',
	'vulkan_editor/swapchain.cpp',
	'vulkan_editor/pipeline.cpp',
	'vulkan_editor/model.cpp',
	'vulkan_editor/physicalDevice.cpp',
	'vulkan_editor/logicalDevice.cpp',
	'vulkan_editor/instance.cpp',
	'vulkan_editor/renderpass.cpp',
	'vulkan_editor/template_loader.cpp',
)

# Source Files
source_files = files('main.cpp') + files('libs/tinyfiledialogs.cpp', 'libs/tinyxml2.cpp') + vulkan_files + imgui_files + editor_files

inja_dep = dependency('inja', required: false)
if not inja_dep.found()
    inja_sp = subproject('inja')
    inja_dep = inja_sp.get_variable('inja_dep')
endif

# Executable
executable(
  'main', source_files,
  dependencies: [sdl3, vulkan, glm, inja_dep],
  include_directories: ['libs', 'imgui', 'imgui/backends'],
  #cpp_args: ['-DNDEBUG']
)
