{pkgs}: {
  deps = [
    pkgs.qt5.full
    pkgs.gcc
    pkgs.qt5.qttools
    pkgs.qt5.qtbase
    pkgs.pkg-config
    pkgs.cmake
  ];
}
