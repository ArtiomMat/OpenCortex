OpenCortex
==========

Goal
----
A library that supports CPU ran AI models, dynamic multi-core utilization to it's fullest.

Currently I want to decompress a lossly compressed sound file with AI, so I don't need many models, just 1D Deconvolution and Neural Networks, but below is a list of models I may implement later. probably not though, depends on how I feel after I am done with the other shit.

Will be awesome if I made Convolution(1D/2D), Deconvolution(2D), Biological based model I was planning a while ago but development halted when I realized I was making, be it quite exotic, slower neural networks.

As of now
---------
Full support of PNG, JPG and intermediete image manipulation, useless for my purpose but why not lol.
Neural networks, generating with random params, loading and unloading them from disk, running them. That's the easy part though, working on fitting algorithm.

MNIST_Test_Source
=================

Contains code for fitting an AI to the handwritten digits using the mnist database, it requires the 4 files that can be obtained from the official website, renamed like so:

mnist.img - The train images
mnist.lbl - The train labels
mnist_t.img - The test images
mnist_t.lbl - The test labels

This code was/is necessary to make sure the ANN algorithms work well.
