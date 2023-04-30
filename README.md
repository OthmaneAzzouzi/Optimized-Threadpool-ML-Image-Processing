# Optimized Threadpool for ML Image Processing (One Core per Thread)

This personal project presents an optimized ThreadPool implementation for image processing as a pre-processing step in machine learning applications. The ThreadPool-related functions have been developed using Pthread condition variables, ensuring efficient image processing with one processor per thread.

# Overview
The Optimized-Threadpool-ML-Image-Processing project is designed to process hundreds of images by applying filters to detect contours. The program lists PNG images in the input directory, applies the desired effects, and saves the results in the output directory.

# Features

The implemented ThreadPool allows for efficient use of all available processors in the machine. By reusing threads and reducing overhead, the ThreadPool speeds up the image processing tasks.

ThreadPool functions include:

1. `threadpool_create()`
2. `threadpool_add_task()`
3. `threadpool_join()`
4. `process_multithread()`

# Dependencies

Ubuntu:

```
  apt-get install build-essential cmake libpng-dev
```

Manual compilation:

```
cmake -S . -B build
cd build
make 
make test
```
# Usage

1. Download test images: `./data/fetch.sh`
2. Add the program to the PATH for simplicity: source `./build/env.sh`
3. Run the main program with input and output image directories: `ieffect --input 
data/ --output results/`
