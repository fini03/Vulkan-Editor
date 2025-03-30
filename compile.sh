#!/bin/bash

# --- Compile Shaders ---
SHADER_SRC_DIR="./shaders"
SHADER_OUT_DIR="./shaders"

# Ensure shader directories exist
mkdir -p "$SHADER_OUT_DIR"

# Compile all .slang shaders
if ! command -v /opt/shader-slang-bin/bin/slangc &> /dev/null; then
    echo "Error: slangc not found."
    exit 1
fi

echo "Compiling shaders..."
for file in "$SHADER_SRC_DIR"/*.slang; do
    if [[ -f "$file" ]]; then
        output="$SHADER_OUT_DIR/$(basename "${file%.slang}.spv")"
        echo "Compiling $file -> $output"
        /opt/shader-slang-bin/bin/slangc -target spirv -o "$output" "$file"
        if [[ $? -eq 0 ]]; then
            echo "✅ Shader compiled successfully: $output"
        else
            echo "❌ Failed to compile shader: $file"
            exit 1
        fi
    fi
done

# --- Compile C++ Code with ImGui ---
# Define source files and output binary
IMGUI_DIR="./imgui"  # Path to your cloned ImGui repository

SOURCE="renderer.cpp \
    ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp \
    ${IMGUI_DIR}/imgui_widgets.cpp ${IMGUI_DIR}/imgui_tables.cpp \
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp \
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"

OUTPUT="code"

# Compiler and flags
CXX=g++
CXXFLAGS="-std=c++17 -I/usr/include/SDL2 -I/usr/include/vulkan -DNDEBUG -O3 \
          -I${IMGUI_DIR} -I${IMGUI_DIR}/backends"
LDFLAGS="-L/usr/lib -lSDL2 -lvulkan"

# Compile the program in release mode
echo "Compiling C++ source files in RELEASE mode..."
$CXX $CXXFLAGS $SOURCE $LDFLAGS -o $OUTPUT

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "🎉 Compilation successful! Run ./$OUTPUT"
else
    echo "💥 Compilation failed. Check errors above."
    exit 1
fi
