# BCF Tools #

This repository contains a few programs to allow extraction of EBSD Data from the Bruker Esprit .bcf files.

## unbcf ##

The `unbcf` program is a general tool to unpack a non-compressed and non-encrypted .bcf file into a folder. The program only requires 2 arguments, the input .bcf file (or SFS file for that matter) and a directory to place the contents. A subfolder will be created for you inside of the given output folder that has the name of the input file (without the extension)

The SFS Reader code were heavily influenced from the [HyperSpy](https://hyperspy.org/) project.
