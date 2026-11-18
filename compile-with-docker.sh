#!/usr/bin/env bash
set -euo pipefail

# ---------------------------------------------
# Usage:
#   ./compile-with-docker.sh [Preset] [CMake options...]
# Examples:
#   ./compile-with-docker.sh Default
#   ./compile-with-docker.sh Custom -DENABLE_SPECTRUM=ON
#   ./compile-with-docker.sh All        # Default + Full
# Default preset: "Custom"
# ---------------------------------------------

IMAGE=uvk1-uvk5v3
PRESET=${1:-Custom}
shift || true  # remove preset from arguments if present

# Any remaining args will be treated as CMake cache variables
EXTRA_ARGS=("$@")

# ---------------------------------------------
# Validate preset name
# ---------------------------------------------
if [[ ! "$PRESET" =~ ^(Default|Full|Custom|All)$ ]]; then
  echo "❌ Unknown preset: '$PRESET'"
  echo "Valid presets are: Default, Full, Custom, All"
  exit 1
fi

# ---------------------------------------------
# Build the Docker image (only needed once)
# ---------------------------------------------
if [[ "$(docker images -q $IMAGE)" == "" ]]; then
  echo "Building Docker image..."
  docker build -t "$IMAGE" -f Dockerfile-slim .
fi

# ---------------------------------------------
# Clean existing CMake cache to ensure toolchain reload
# ---------------------------------------------
rm -rf build
export MSYS_NO_PATHCONV=1
# ---------------------------------------------
# Function to build one preset
# ---------------------------------------------
build_preset() {
  local preset="$1"
  echo ""
  echo "=== 🚀 Building preset: ${preset} ==="
  echo "---------------------------------------------"
  docker run --rm \
    -u $(id -u):$(id -g) \
    -v "$PWD":/src -w /src "$IMAGE" \
    bash -c "which arm-none-eabi-gcc && arm-none-eabi-gcc --version && \
             cmake --preset ${preset} ${EXTRA_ARGS[@]+"${EXTRA_ARGS[@]}"} && \
             cmake --build --preset ${preset} -j"
  echo "✅ Done: ${preset}"
}

# ---------------------------------------------
# Handle 'All' preset
# ---------------------------------------------
if [[ "$PRESET" == "All" ]]; then
  PRESETS=(Default Full)
  for p in "${PRESETS[@]}"; do
    build_preset "$p"
  done
  echo ""
  echo "🎉 All presets built successfully!"
else
  build_preset "$PRESET"
fi
