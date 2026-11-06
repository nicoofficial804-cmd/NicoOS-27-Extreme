#!/usr/bin/env bash
# shellcheck disable=SC2034
#
# NicoOS 27 Extreme — definizione del profilo archiso.

iso_name="NicoOS-27-Extreme"
iso_label="NICOOS_27"
iso_publisher="NicoOS <https://nicoos.dev>"
iso_application="NicoOS 27 Extreme — desktop live e installer"
iso_version="27"
install_dir="arch"
buildmodes=('iso')
bootmodes=('bios.syslinux'
           'uefi.systemd-boot')
pacman_conf="pacman.conf"
airootfs_image_type="squashfs"
airootfs_image_tool_options=('-comp' 'zstd' '-Xcompression-level' '12' '-b' '1M')
bootstrap_tarball_compression=('zstd' '-c' '-T0' '--auto-threads=logical' '--long' '-19')
file_permissions=(
  ["/root"]="0:0:750"
  ["/usr/local/bin/nicoos-session"]="0:0:755"
  ["/usr/local/bin/nicoos-emergency-shell"]="0:0:755"
  ["/usr/local/bin/nicoos-installer"]="0:0:755"
  ["/usr/local/bin/nicoos-ensure-shell"]="0:0:755"
  ["/usr/local/bin/nicoos-target-prepare"]="0:0:755"
  ["/usr/local/bin/nicoos-target-finalize"]="0:0:755"
  ["/root/customize_airootfs.sh"]="0:0:755"
)
