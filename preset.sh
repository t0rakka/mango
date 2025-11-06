#!/bin/bash

print_usage() {
  echo "Usage: $0 <preset> [static|shared] [release|debug] [gcc|clang]"
}

# Required: preset name
PRESET="$1"
shift

if [[ -z "$PRESET" ]]; then
  echo "Error: Missing required <preset>"
  print_usage
  exit 1
fi

# Build the command
CMD=(cmake --preset "$PRESET")

# Parse remaining args (position-insensitive)
for arg in "$@"; do
  case "$arg" in
    static)
      CMD+=("-DBUILD_SHARED_LIBS=OFF")
      ;;
    shared)
      CMD+=("-DBUILD_SHARED_LIBS=ON")
      ;;
    release|debug)
      CMD+=("-DCMAKE_BUILD_TYPE="$arg"")
      ;;
    gcc)
      CMD+=("-DCMAKE_C_COMPILER=gcc")
      CMD+=("-DCMAKE_CXX_COMPILER=g++")
      ;;
    clang)
      CMD+=("-DCMAKE_C_COMPILER=clang")
      CMD+=("-DCMAKE_CXX_COMPILER=clang++")
      ;;
    *)
      # Add directly into the command line
      CMD+=("$arg")
      ;;
  esac
done

# Run the command
"${CMD[@]}"
