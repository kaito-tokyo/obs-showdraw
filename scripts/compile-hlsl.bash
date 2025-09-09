#!/bin/bash

# Define the input HLSL file
HLSL_FILE="$(dirname "$0")"/../data/effects/drawing-hlsl.effect

# Define the output directory for compiled shaders
OUTPUT_DIR="$(dirname "$0")"/../build/shaders

# Create the output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Array of pixel shader entry points
PIXEL_SHADERS=(
    "PSDraw"
    "PSExtractLuminance"
    "PSHorizontalMedian3"
    "PSHorizontalMedian5"
    "PSHorizontalMedian7"
    "PSHorizontalMedian9"
    "PSVerticalMedian3"
    "PSVerticalMedian5"
    "PSVerticalMedian7"
    "PSVerticalMedian9"
    "PSCalculateMotionMap"
    "PSCalculateHorizontalMotionMap"
    "PSCalculateVerticalMotionMap"
    "PSMotionAdaptiveFiltering"
    "PSApplySobel"
    "PSFinalizeSobelMagnitude"
    "PSSuppressNonMaximum"
    "PSHysteresisClassify"
    "PSHysteresisPropagate"
    "PSHysteresisFinalize"
    "PSHorizontalErosion"
    "PSVerticalErosion"
    "PSHorizontalDilation"
    "PSVerticalDilation"
)

# Loop through each pixel shader and compile it
for SHADER_NAME in "${PIXEL_SHADERS[@]}"; do
    OUTPUT_FILE="$OUTPUT_DIR/${SHADER_NAME}.cso"
    echo "Compiling $SHADER_NAME to $OUTPUT_FILE..."
    dxc -T ps_6_0 -E "$SHADER_NAME" "$HLSL_FILE" -Fo "$OUTPUT_FILE"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to compile $SHADER_NAME"
        exit 1
    fi
done

echo "All pixel shaders compiled successfully."
