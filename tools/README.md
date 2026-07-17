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

`build_patch_download.sh` follows the GUI project's `build_patch_download.sh`
flow. It builds and packages first, then calls the shared ISP project's
`macos-copy-and-download.sh`. The default ISP location is
`/Volumes/mpushare/mpushare/macos_workspace/isp-9star`; override it when
needed: `ISP_TOOL=/path/to/macos-copy-and-download.sh ./build_patch_download.sh /dev/ttyUSB0`.

Use `--force` only when replacing files created by a previous generation.
The build assumes xPack `riscv-none-elf-gcc` is installed at the default path
used by the GUI sample; pass `TOOLCHAIN_DIR=/path/to/xpack` to `make` to change it.
