# TunaOS KDE Installer — Qt6 Widgets frontend for fisherman

**Thin Qt6/C++ wizard** that drives the [fisherman](https://github.com/tuna-os/fisherman) bootc install backend.

## Workflow

1. **Welcome** — brief intro
2. **Disk Selection** — `lsblk -J` lists available disks; user picks one
3. **Confirm** — summary of choices (disk, encryption, hostname, image)
4. **Install** — writes recipe JSON, runs `fisherman <recipe.json>`, streams output
5. **Done** — success/failure with close button

## Build

```bash
# Dependencies (Fedora)
sudo dnf install -y qt6-qtbase-devel cmake gcc-c++

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run
./build/tuna-installer-kde
```

## Recipe

The installer writes a JSON recipe that fisherman consumes:

```json
{
  "disk": "/dev/nvme0n1",
  "filesystem": "btrfs",
  "btrfsSubvolumes": true,
  "encryption": {"type": "luks-passphrase", "passphrase": "…"},
  "image": "ghcr.io/tuna-os/albacore:gnome",
  "additionalImageStores": ["/usr/share/tuna-installer/oci-store"],
  "distroID": "tunaos",
  "selinuxDisabled": true,
  "hostname": "tunaos"
}
```

Encryption types: `none`, `luks-passphrase`, `tpm2-luks`, `tpm2-luks-passphrase`.
On a live ISO, `image` may be omitted — bootc installs the running container
(offline, no download). See `../INSTALLER-FRONTENDS.md` for the full contract.

```json
```

## License

GPL-3.0-only
