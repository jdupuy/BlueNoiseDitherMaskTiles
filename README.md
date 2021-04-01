# Blue-Noise Dither Mask Tiles Generator

[![Build Status](https://travis-ci.org/jdupuy/BlueNoiseDitherMaskTiles.svg?branch=master)](https://travis-ci.org/jdupuy/BlueNoiseDitherMaskTiles) [![Build status](https://ci.appveyor.com/api/projects/status/x65qoyhppsmpkxs1?svg=true)](https://ci.appveyor.com/project/jdupuy/bluenoisedithermasktiles)

### Details
This repository provides a generator for dither masks with blue-noise power spectrums. The generator is based on the algorithm introduced in the paper "Void-and-cluster method for dither array generation", by Ulichney (see References section).
The generated masks are tileable and can have arbitrary resolutions. 
The generator runs on the GPU and recquires an OpenGL4.5 compatible card.
Below is an example of a 256x256 dither mask along with its power spectrum:

![alt text](preview.png "Mask") ![alt text](spectrum.png "Power Spectrum")

A set of precomputed masks can be downloaded from the examples/ repository.

### References

 Robert A. Ulichney "Void-and-cluster method for dither array generation", Proc. SPIE 1913, Human Vision, Visual Processing, and Digital Display IV, (8 September 1993); https://doi.org/10.1117/12.152707 

### Cloning

Clone the repository and all its submodules using the following command:
```sh
git clone --recursive git@github.com:jdupuy/BlueNoiseDitherMaskTiles.git
```

If you accidentally omitted the `--recursive` flag when cloning the repository you can retrieve the submodules like so:
```sh
git submodule update --init --recursive
```

### License
The code from this repository is released in public domain.
