# Optimized Threadpool for ML Image Processing (One Processor per Thread)

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

```
  apt-get install build-essential cmake libpng-dev
```

# Manual Compilation:

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

# Important Note on Code Conventions

Please be aware that in the source code for this program, variable names, function names, and comments are primarily written in French. I understand that this might pose some challenges for users who are not familiar with the French language. However, I have ensured that the user documentation, such as this README, is written in English for broader accessibility.

