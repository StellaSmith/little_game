FROM docker.io/fedora:37
RUN dnf install -y \
$(echo "build tools" > /dev/null) \
    git ccache mold cmake gcc clang mingw{32,64}-gcc{-c++,} python3-pip \
$(echo "required by openssl/1.1.1m" > /dev/null) \
    perl \ 
$(echo "required by bison/3.7.1" > /dev/null) \
    texinfo \
$(echo "required by opengl/system" > /dev/null) \
    libglvnd-devel \
$(echo "required by egl/system" > /dev/null) \
    mesa-libEGL-devel \
$(echo "required by xorg/system" > /dev/null) \
    libxcb-devel \
    libfontenc-devel \
    libXaw-devel \
    libXcomposite-devel \
    libXcursor-devel \
    libXdmcp-devel \
    libXft-devel \
    libXtst-devel \
    libXinerama-devel \
    libxkbfile-devel \
    libXrandr-devel \
    libXres-devel \
    libXScrnSaver-devel \
    libXvMC-devel \
    xorg-x11-xtrans-devel \
    xcb-util-wm-devel \
    xcb-util-image-devel \
    xcb-util-keysyms-devel \
    xcb-util-renderutil-devel \
    libXdamage-devel \
    libXxf86vm-devel \
    libXv-devel \
    xkeyboard-config-devel \
    xcb-util-devel \
    libuuid-devel && \
    rm -rf /var/cache/dnf
# so we can have ~/.local/bin in path
SHELL ["/usr/bin/bash", "--login", "-c"]
RUN git clone --depth=1 https://github.com/tpoechtrager/wclang.git /usr/src/wclang && \
    cmake -S /usr/src/wclang -B /tmp/wclang-build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build /tmp/wclang-build --parallel && \
    cmake --install /tmp/wclang-build && \
    rm -rf /tmp/wclang-build /usr/src/wclang && \
    ln -s ../../ccache /usr/lib64/ccache/x86_64-w64-mingw32-clang && \
    ln -s ../../ccache /usr/lib64/ccache/x86_64-w64-mingw32-clang++ && \
    ln -s ../../ccache /usr/lib64/ccache/i686-w64-mingw32-clang && \
    ln -s ../../ccache /usr/lib64/ccache/i686-w64-mingw32-clang++
RUN pip install --user --upgrade pip setuptools wheel && \
    pip install --user --upgrade "conan==1.*" && \
    rm -rf ~/.cache/{ccache,pip}
RUN conan profile new --detect default && conan profile update settings.compiler.libcxx=libstdc++11 default

# i would recommend mounting /root/.conan/data and /root/.cache/ccache into a volume outside the 
# container to be able to cache it for later
