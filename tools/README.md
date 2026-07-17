# TD1601 Makefile generator

Generate a macOS/xPack build for any CDK solution that has `package.yaml` and
`project.cdkproj`:

```sh
python3 tools/generate_td1601_makefiles.py projects/drivers/gpio/gpio_toggle
cd projects/drivers/gpio/gpio_toggle
make
bash aft_build_macos.sh
# Build + package + copy to the shared ISP project and burn on Ubuntu's serial port.
./build_patch_download.sh /dev/ttyUSB0
```

The generator reads CDK package references from `project.cdkproj`, recursively
resolves `depends` in each `package.yaml`, expands `source_file` globs and
combines public include directories, `def_config`, `ldflag`, and `libs`.
It produces `Makefile`, `aft_build_macos.sh`, `build_patch_download.sh`, and
the required xPack assembler compatibility header under `generated/`.

`build_patch_download.sh` follows the verified GUI/lierda download flow. It
builds and packages first, then calls the generated project's own
`utilities/macos-copy-and-download.sh`. That helper stages the image in the
shared ISP project and forwards the explicit image path to Ubuntu. Override it
when needed with
`ISP_TOOL=/path/to/macos-copy-and-download.sh ./build_patch_download.sh /dev/ttyUSB0`.

Generated Makefiles contain SDK-relative source/include paths. Startup object
names retain the `startup.o` suffix required by `gcc_flash.ld`.

Use `--force` only when replacing files created by a previous generation.
The build assumes xPack `riscv-none-elf-gcc` is installed at the default path
used by the GUI sample; pass `TOOLCHAIN_DIR=/path/to/xpack` to `make` to change it.
