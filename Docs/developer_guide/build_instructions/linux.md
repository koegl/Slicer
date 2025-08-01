# GNU/Linux systems

The instructions to build Slicer for GNU/Linux systems are slightly different
depending on the Linux distribution and the specific configuration of the
system. In the following sections, you can find instructions that will work for
some of the most common Linux distributions in their standard configuration. If
you are using a different distribution, you can use [these instructions](./linux.md#any-distribution)
to adapt the process to your system. You can also ask questions
related to the building process in the [Slicer forum](https://discourse.slicer.org).

## Prerequisites

First, you need to install the tools that will be used for fetching the source
code of Slicer, generating the project files, and building the project.

- Git for fetching the code and version control.
- GNU Compiler Collection (GCC) for code compilation.
- CMake for configuration/generation of the project.
  - (Optional) CMake curses GUI to configure the project from the command line.
  - (Optional) CMake Qt GUI to configure the project through a GUI.
- GNU Make
- GNU Patch

In addition, Slicer requires a set of support libraries that are not included as
part of the *superbuild*:

- Qt5 with the components listed below. Qt version 5.15.2 is recommended; other Qt versions are not tested and may cause build errors or may cause problems when running the application.
  - Multimedia
  - UiTools
  - XMLPatterns
  - SVG
  - WebEngine
  - Script
  - X11Extras
  - Private
- libXt

### Debian 12 Bookworm (Stable) and Bullseye 11 (OldStable)

Install the development tools and the support libraries:

```console
sudo apt update && sudo apt install git build-essential cmake cmake-curses-gui cmake-qt-gui \
  qtmultimedia5-dev qttools5-dev libqt5xmlpatterns5-dev libqt5svg5-dev qtwebengine5-dev qtscript5-dev \
  qtbase5-private-dev libqt5x11extras5-dev libxt-dev libssl-dev
```

:::{note}
The CMake version currently included in Debian 12 Bookworm (Stable) is not compatible with the current development version of Slicer.
For more details, see the Slicer [CMakeLists.txt](https://github.com/Slicer/Slicer/blob/98c092edb8f5a274277d2e486a4f7e584f58605e/CMakeLists.txt#L3-L5) file. On Debian 12 Bookworm (Stable), you will need to upgrade CMake manually by downloading CMake version >= 3.25.3 and < 4 from the [CMake website](https://cmake.org/download/) and following the CMake installation instructions. **Slicer build scripts are not yet compatible with CMake 4.x**

:::

### Ubuntu 24.04 (Noble Numbat)

Install the development tools and the support libraries:

```console
sudo apt update && sudo apt install git git-lfs build-essential \
  libqt5x11extras5-dev qtmultimedia5-dev libqt5svg5-dev qtwebengine5-dev libqt5xmlpatterns5-dev qttools5-dev qtbase5-private-dev \
  libxt-dev
```

:::{note}
The CMake version currently included in Ubuntu 24.04 is CMake 3.28.3 which is compatible with the current development version of Slicer. **Last time tested: 2025-05-27.**
:::

### Ubuntu 23.04 (Lunar Lobster)

Install the development tools and the support libraries:

```console
sudo apt update && sudo apt install git git-lfs build-essential \
  libqt5x11extras5-dev qtmultimedia5-dev libqt5svg5-dev qtwebengine5-dev libqt5xmlpatterns5-dev qttools5-dev qtbase5-private-dev \
  libxt-dev
```

Install CMake manually by downloading CMake >=3.25.3 and < 4 from the [CMake website](https://cmake.org/download/)
and by following the CMake installation instructions. **Slicer build scripts are not yet compatible with CMake 4.x**

:::{note}
The CMake version currently included in Ubuntu 23.04 is CMake 3.25.1 (see [here](https://packages.ubuntu.com/lunar/cmake))
and is not compatible with the current development version of Slicer.
For more details, see the Slicer [CMakeLists.txt](https://github.com/Slicer/Slicer/blob/98c092edb8f5a274277d2e486a4f7e584f58605e/CMakeLists.txt#L3-L5) file.
:::

### Ubuntu 22.04 (Jammy Jellyfish)

Install the development tools and the support libraries:

```console
sudo apt update && sudo apt install git build-essential \
  cmake cmake-curses-gui cmake-qt-gui \
  libqt5x11extras5-dev qtmultimedia5-dev libqt5svg5-dev qtwebengine5-dev libqt5xmlpatterns5-dev qttools5-dev qtbase5-private-dev \
  qtbase5-dev qt5-qmake
```

### Ubuntu 21.10 (Impish Indri)

Install the development tools and the support libraries:

```console
sudo apt update && sudo apt install git build-essential \
  cmake cmake-curses-gui cmake-qt-gui \
  libqt5x11extras5-dev qtmultimedia5-dev libqt5svg5-dev qtwebengine5-dev libqt5xmlpatterns5-dev qttools5-dev qtbase5-private-dev \
  libxt-dev libssl-dev
```

### ArchLinux

:::{warning}
ArchLinux uses a rolling-release package distribution approach. This means that the versions of the packages will change over time and the following instructions might not be actual. **Last time tested: 2024-01-01.**
:::

You could build Slicer using the `PKGBUILD` from AUR: [3dslicer](https://aur.archlinux.org/packages/3dslicer) and [3dslicer-git](https://aur.archlinux.org/packages/3dslicer-git).

### CentOS 7
:::{note}
Slicer built on CentOS 7 will be available for many Linux distributions and releases
:::

Install Qt and CMake as described in [Any Distribution](./linux.md#any-distribution) section.

Since by default CentOS 7 comes with `gcc 4.8.5` only having [experimental support for C++14](https://gcc.gnu.org/onlinedocs/gcc-4.8.5/gcc/C-Dialect-Options.html#C-Dialect-Options), the following allows to install and activate the `devtoolset-11` [providing](https://access.redhat.com/documentation/en-us/red_hat_developer_toolset/11/html/11.0_release_notes/dts11.0_release#Changes_in_DTS) `gcc 11.2.1` [supporting C++20](https://en.cppreference.com/w/cpp/compiler_support/20):

```console
sudo yum install centos-release-scl
sudo yum install devtoolset-11-gcc*
scl enable devtoolset-11 bash         # activation is needed for every terminal session
```

Install pre-requisites:
```console
sudo yum install patch mesa-libGL-devel libuuid-devel
```

### Any Distribution

This section describes how to install Qt as distributed by *The QT Company*, which can be used for any GNU/Linux distribution.

:::{important}
This process requires an account in [qt.io](https://qt.io)

:::

Download the Qt Linux online installer and make it executable:

```console
 curl -LO https://download.qt.io/official_releases/online_installers/qt-online-installer-linux-x64-online.run
 chmod +x qt-online-installer-linux-x64-online.run
```
You can run the installer and follow the instructions in the GUI. Keep in mind that the components needed by 3D Slicer are: `qt.qt5.5152.gcc_64`, `qt.qt5.5152.qtwebengine` and `qt.qt5.5152.qtwebengine.gcc_64`.

Alternatively, you can request the installation of the components with the following command (you will be prompted for license agreements and permissions):

```console
export QT_ACCOUNT_LOGIN=<set your qt.io account email here>
export QT_ACCOUNT_PASSWORD=<set your password here>
./qt-online-installer-linux-x64-online.run \
  install \
    qt.qt5.5152.gcc_64 \
    qt.qt5.5152.qtwebengine \
    qt.qt5.5152.qtwebengine.gcc_64 \
  --root /opt/qt \
  --email $QT_ACCOUNT_LOGIN \
  --pw $QT_ACCOUNT_PASSWORD
```
:::{hint}
When configuring the Slicer build project, the CMake variable `Qt5_DIR` need to be set using the full path to the Qt5 installation directory ending with `5.15.2/gcc_64/lib/cmake/Qt5`. For example, assuming you installed Qt in `/opt/qt`, you may use `cmake -DCMAKE_BUILD_TYPE:STRING=Release -DQt5_DIR:PATH=/opt/qt/5.15.2/gcc_64/lib/cmake/Qt5 ../Slicer`.

:::

## Checkout Slicer source files

The recommended way to obtain the source code of Slicer is cloning the repository using `git`:

```console
git clone https://github.com/Slicer/Slicer.git
```

This will create a `Slicer` directory containing the source code of Slicer.
Hereafter we will call this directory the `source directory`.

:::{tip}
It is highly recommended to **avoid** the use of the **space** character in the name of the `source directory` or any of its parent directories.

:::

After obtaining the source code, we need to set up the development environment:

```console
cd Slicer
./Utilities/SetupForDevelopment.sh
cd ..
```

% TODO: Link to the readthedocs equivalent of https://www.slicer.org/wiki/Documentation/Nightly/Developers/DevelopmentWithGit

## Configure and generate the Slicer build project files

Slicer is highly configurable and multi-platform. To support this,
Slicer needs a configuration of the build parameters before the build process
takes place. In this configuration stage, it is possible to adjust variables
that change the nature and behavior of its components. For instance, the type
of build (Debug or Release mode), whether to use system-installed libraries,
let the build process fetch and compile own libraries, or enable/disable some of
the software components and functionalities of Slicer.

The following folders will be used in the instructions below:

| Folder          | Path   |
|-----------------|--------|
| **source**      | `~/Slicer` |
| **build**       |  `~/Slicer-SuperBuild-Debug` |
| **inner-build** |  `~/Slicer-SuperBuild-Debug/Slicer-build` |

To obtain a default configuration of the Slicer build project, create the **build** folder and use `cmake`:

```console
mkdir Slicer-SuperBuild-Debug
cd Slicer-SuperBuild-Debug
cmake ../Slicer
```

It is possible to change variables with `cmake`. In the following example we
change the built type (Debug as default) to Release:

```console
cmake -DCMAKE_BUILD_TYPE:STRING=Release ../Slicer
```

:::{warning}
On Debian 12 Bookworm (Stable), the included OpenSSL version (3.0.9) is not compatible with the OpenSSL versions (1.0 - 1.1) used in Slicer, and attempting to run Slicer will emit the following warning, indicating that SSL support is disabled:
```
qt.network.ssl: Incompatible version of OpenSSL (built with OpenSSL >= 3.x, runtime version is < 3.x)
[SSL] SSL support disabled - Failed to load SSL library !
[SSL] Failed to load Slicer.crt
QSslSocket::connectToHostEncrypted: TLS initialization failed
```
To enable SSL, one can use the system OpenSSL as follows:
```console
cmake -DSlicer_USE_SYSTEM_OpenSSL=ON ../Slicer
```
:::

:::{admonition} Tip -- Interfaces to change 3D Slicer configuration variables

Instead of `cmake`, one can use `ccmake`, which provides a text-based interface, or `cmake-gui`, which provides a graphical user interface. These applications will also provide a list of variables that can be changed.

:::

:::{admonition} Tip -- Speed up 3D Slicer build with `ccache`

`ccache` is a compiler cache that can speed up subsequent builds of 3D Slicer. This can be useful if 3D Slicer is built often and there are no large divergences between subsequent builds. This requires `ccache` installed on the system (e.g., `sudo apt install ccache`).

The first time `ccache` is used, the compilation time can marginally increase as it includes the first caching. After the first build, subsequent build times will decrease significantly.

`ccache` is not detected as a valid compiler by the 3D Slicer building process. You can generate local symbolic links to disguise the use of `ccache` as valid compilers:

```console
ln -s /usr/bin/ccache ~/.local/bin/c++
ln -s /usr/bin/ccache ~/.local/bin/cc
```

Then, the Slicer build can be configured to use these compilers:

```console
cmake \
  -DCMAKE_BUILD_TYPE:STRING=Release \
  -DCMAKE_CXX_COMPILER:STRING=$HOME/.local/bin/c++ \
  -DCMAKE_C_COMPILER:STRING=$HOME/.local/bin/cc \
  ../Slicer
```

:::

## Build Slicer

Once the Slicer build project files have been generated, the Slicer project can
be built by running this command in the **build** folder

```console
make
```

:::{admonition} Tip -- Parallel build

Building Slicer will generally take a long time, particularly on the first build or upon code/configuration changes. To help speed up the process, one can use `make -j<N>`, where `<N>` is the number of parallel builds. As a rule of thumb, many use the `number of CPU threads - 1` as the number of parallel builds.

:::

:::{warning}

Increasing the number of parallel builds generally increases the memory required for the build process. In the event that the required memory exceeds the available memory, the process will either fail or start using swap memory, which may make the system freeze.

:::

:::{admonition} Tip -- Error detection during parallel build

Using parallel builds makes finding compilation errors difficult due to the fact that all parallel build processes use the same screen output, as opposed to sequential builds, where the compilation process will stop at the error. A common technique to have parallel builds and easily find errors is to launch a parallel build followed by a sequential build. For the parallel build, it is advised to run `make -j<N> -k` to have the parallel build keep going as far as possible before doing the sequential build with `make`.

:::

## Run Slicer

After the building process has successfully completed, the executable file to
run Slicer will be located in the **inner-build** folder.

The application can be launched by these commands:

```console
cd Slicer-build
./Slicer`
```

## Test Slicer

After building, run the tests in the **inner-build** folder.

Type the following (you can replace 4 with the number of processor cores in the computer):

```console
ctest -j4
```

## Package Slicer

Start a terminal and type the following in the **inner-build** folder:

```console
make package
```

## Common errors

See a list of issues common to all operating systems on the [Common errors](common_errors.md) page.
