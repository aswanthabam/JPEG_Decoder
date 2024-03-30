# JPEG Decoder

Project Aimed for practicing JPEG Compression Algorithm. In this we use c++ for decoding a JPEG imaging and displaying it.

### Current Progress:

- [x] Read Markers
- [x] Read Image Headers including Huffman Tables, Quantization Table and other image details.
- [x] Decode huffman coded data and convert into MCUs
- [x] Convert into bmp file with YCbCr
- [ ] Apply Inverse Quantization
- [ ] Apply Inverse DCT
- [ ] Apply YCbCr to RGB Convertion
- [ ] Display image

## Requirements

- Better run it on linux based devices, because we use X11 lib to display image
- You want to install xlib on our system
    ```bash
    sudo apt-get update
    sudo apt-get upgrade
    sudo apt-get install xorg
    ```

### Run 

```bash
g++ jpeg.cpp -lX11
./a.out <image_path>
```
