# Texture Atlas Packer

Texture Atlas Packer is a CLI tool designed with simplicity in mind to pack directories into texture atlases. 
A lot of customization points are provided for more complex use cases. [TeamHypersomnia/rectpack2D](https://github.com/TeamHypersomnia/rectpack2D) 
provides the rectangle packing algorithm used by this tool.

# Usage
Folder structure.
```
dir-0/
    a.png
    b.jpg
dir-1/
    a.png
    subdir/
        b.tga
        c.bmp
```

The command below will generate texture atlas from images in the dir-0 and dir-1, the bin size is 2048x2048,
bins and config will be saved in /atlas/ folder. The second `a.png` will be skipped, since it has a duplicate 
name in the atlas-space image dictionary.
```sh 
texture-atlas-packer -d /dir-0 -d /dir-1 -o /atlas -c /atlas/config.json -s 2048
```
The output contains bins and the .json config file describing, what is in generated bins.
```json
{
    "version": 1,
    "bin-textures": [
        "atlas-01.png",
        "atlas-02.png"
    ],
    "images": {
        "a": {
            "bin": 0,
            "height": 641,
            "width": 962,
            "x": 0,
            "y": 0
        },
        "b": { ... },
        "subdir/b": { ... },
        "subdir/c": {
            "bin": 1
            ...
        }
    }
}
```

# Dependencies
* [TeamHypersomnia/rectpack2D](https://github.com/TeamHypersomnia/rectpack2D)
* [CLIUtils/CLI11](https://github.com/CLIUtils/CLI11)
* [nothings/stb](https://github.com/nothings/stb)
* [nlohmann/json](https://github.com/nlohmann/json)

# Licenses
Texture Atlas Packer is licensed under the [MIT License](/LICENSE).

TeamHypersomnia/rectpack2D is licensed under the [MIT License](https://github.com/TeamHypersomnia/rectpack2D/blob/master/LICENSE)

CLIUtils/CLI11 is licensed under the [Custom License](https://github.com/CLIUtils/CLI11/blob/main/LICENSE)

nothings/stb is licensed under the [MIT License](https://github.com/nothings/stb/blob/master/LICENSE)

nlohmann/json is licensed under the [MIT License](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT)

