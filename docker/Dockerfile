FROM ubuntu
MAINTAINER sott0n <fix7211@gmail.com>

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        git \
        make \
        gdb \
        pkg-config \
        libglib2.0-dev

RUN git clone https://git.qemu.org/git/qemu.git && \
    cd qemu && \
    ./configure --static --disable-system --target-list=riscv64-linux-user --python=/usr/bin/python3.6 && \
    make -j $(nproc) && \
    cp riscv64-linux-user/qemu-riscv64 /usr/bin/ && \
    cd .. && \
    rm -rf qemu
