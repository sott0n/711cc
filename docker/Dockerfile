FROM ubuntu
MAINTAINER sott0n <fix7211@gmail.com>

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        git \
        make \
        gdb \
        pkg-config \
        libglib2.0-dev

# Install qemu user-mode for riscv
RUN git clone https://git.qemu.org/git/qemu.git && \
    cd qemu && \
    ./configure --static --disable-system --target-list=riscv64-linux-user --python=/usr/bin/python3.6 && \
    make -j $(nproc) && \
    cp riscv64-linux-user/qemu-riscv64 /usr/bin/ && \
    cd .. && rm -rf qemu

# Install gcc cross-compiler for riscv
RUN dpkg --add-architecture riscv64 && \
    apt-get install gcc-riscv64-linux-gnu && \
    ln -s /usr/riscv64-linux-gnu/lib/ld-linux-riscv64-lp64d.so.1 /lib

ENV LD_LIBRARY_PATH=/usr/riscv64-linux-gnu/lib/
