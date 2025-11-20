#!/bin/bash

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check for Docker
if ! command_exists docker; then
    echo "Error: Docker is not installed or not in your PATH."
    echo "Please install Docker Desktop: https://www.docker.com/products/docker-desktop/"
    exit 1
fi

echo "Checking for Docker..."
if ! docker info > /dev/null 2>&1; then
  echo "Error: Docker daemon is not running."
  echo "Please start Docker Desktop."
  exit 1
fi

IMAGE_NAME="zerasul/sgdk:latest"

echo "Pulling latest SGDK image..."
docker pull $IMAGE_NAME

# Helper function to run make in docker safely
# We override entrypoint to ensure we can run 'make' explicitly
run_make() {
    docker run --rm --entrypoint /bin/bash -v "$(pwd)":/src -w /src $IMAGE_NAME -c "$1"
}

# Check if TEST flag is passed
if [ "$1" == "test" ]; then
    echo "Building TEST ROM..."
    
    # Clean first
    run_make "make -f Makefile clean"
    
    # Build with TEST_BUILD defined
    # We passed EXTRA_FLAGS to Makefile which appends to OPTIONS
    run_make "make -f Makefile EXTRA_FLAGS=-DTEST_BUILD"
    
    echo "Test Build Complete: out/rom.bin"
    echo "Running this ROM in an emulator will display Test Results."
else
    echo "Building the game..."
    run_make "make -f Makefile"
fi
