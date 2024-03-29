FROM debian:bullseye

ARG USERNAME
ARG ARCH
ARG CROSSBUILD

RUN useradd -m $USERNAME
RUN usermod -aG sudo $USERNAME
RUN usermod --shell /bin/bash $USERNAME

RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y \
    sudo \
    bash-completion \
    locales \
    gnupg2 \
    gdb \
    gdb-multiarch \
    rsync \
    make \
    cmake \
    git \
    nano \
    avahi-utils \
    iputils-ping \
    cmake \
    x11-apps \
    libgl1-mesa-dev \
    xorg-dev

RUN dpkg --add-architecture $ARCH
RUN apt-get update -q
RUN apt-get install -y \
    crossbuild-essential-$ARCH \
    libopenblas64-dev:$ARCH

RUN apt-get upgrade -y

RUN mkdir -p /etc/sudoers.d/
RUN echo "$USERNAME ALL=(ALL:ALL) NOPASSWD: ALL" >> /etc/sudoers.d/$USERNAME
RUN chmod 440 /etc/sudoers.d/$USERNAME
RUN touch /var/lib/sudo/lectured/$USERNAME

ENV PKG_CONFIG_PATH=/usr/lib/${CROSSBUILD}/pkgconfig:/usr/share/pkgconfig
ENV PKG_CONFIG_LIBDIR=/usr/lib/${CROSSBUILD}/pkgconfig:/usr/share/pkgconfig
ENV DISPLAY=:0
ENV ARCH=$ARCH
ENV CROSSBUILD=${CROSSBUILD}
ENV CC=${CROSSBUILD}-gcc
ENV CXX=${CROSSBUILD}-g++
ENV TZ=Asia/Tokyo
RUN sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8

USER $USERNAME