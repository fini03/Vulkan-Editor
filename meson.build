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
  'imgui/backends/imgui_impl_vulkan.cpp'
)

# Vulkan Files
vulkan_files = files(
  'vulkan_editor/vulkan_device.cpp',
  'vulkan_editor/vulkan_swapchain.cpp',
  'vulkan_editor/vulkan_renderpass.cpp',
)

# Source Files
source_files = files('main.cpp') + vulkan_files + imgui_files

# Executable
executable(
  'main', source_files,
  dependencies: [sdl3, vulkan, glm],
  include_directories: ['libs', 'imgui', 'imgui/backends'],
  #cpp_args: ['-DNDEBUG']
)
